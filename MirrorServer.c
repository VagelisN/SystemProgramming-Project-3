#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>	
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>	 
#include <fcntl.h>
#include <unistd.h> 
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "mirror_funcs.h"
#define MESSAGESIZE 512

struct queue* pool_queue=NULL;
pthread_mutex_t gen_mutex=PTHREAD_MUTEX_INITIALIZER;//used to keep stats
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;//prod cons
pthread_cond_t empty=PTHREAD_COND_INITIALIZER;//prod cons
pthread_cond_t full=PTHREAD_COND_INITIALIZER;//prod cons
pthread_cond_t all_done=PTHREAD_COND_INITIALIZER;//transfer done ready for the next one

pthread_mutex_t mutex_array[QUEUE_LENGTH];//prod cons
double bytes_transfered=0;
int files_transfered=0;
int managers_exited=0;
int manager_count=0;
int files_to_transfer=0;
int exit_all=0;
//int num_of_prods=0;
//int num_of_cons=0;



int main(int argc, char **argv)
{
	char c,*dirname,buff[MESSAGESIZE]={'\0'};
	int port,threadnum,sock,cli_sock,i,error,id=0,mflag=0;
	struct sockaddr_in ser,cli;
	pthread_t *thread_array;
	socklen_t client_len;
	struct thread_listnode* thread_list=NULL;
	if(argc!=7){printf("To run: MirrorServer -p <port> -m <dirname> -w <threadnum>\n");exit(1);}
	while((c = getopt (argc, argv, "p:m:w:")) != -1)
	{
		switch(c)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'm':
				dirname=malloc(sizeof(char) * (strlen(optarg)+1));
				strcpy(dirname,optarg);
				if(mkdir(dirname,0777)==-1)printf("Error creating the directory\n");;
				break;
			case 'w':
				threadnum=atoi(optarg);
				if(threadnum <= 0){printf("threadnum has to be > 0\n");exit(1);}
				thread_array=malloc(sizeof(pthread_t)*threadnum);
				break;
			default:
				break;
		}		
	}
	queue_init(&pool_queue);//initialize the shared buffer
	for (i = 0; i <threadnum ; i++){if(error=pthread_create(&thread_array[i],NULL,worker,(void*)dirname)){fprintf(stderr,"pthread_create %s\n",strerror(error));exit(1);}}//start the workers
	for(i = 0; i <QUEUE_LENGTH; i++)pthread_mutex_init(&mutex_array[i], 0);
	if((sock=socket(PF_INET, SOCK_STREAM, 0)) ==-1){perror("socket");exit(-1);}
	ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    ser.sin_port = htons(port);
    if(bind(sock, (struct sockaddr*)&ser,sizeof(ser)) == -1){perror("bind");exit(-1);}
	if(listen(sock,5) == -1){perror("listen");exit(-1);}
	while(1)//wait for connections
	{
		client_len=sizeof(cli);
		printf("Waiting for Connection\n");
		if((cli_sock = accept(sock, (struct sockaddr*)&cli, &client_len)) ==-1){perror("accept");exit(-1);}
		/*for every command given from the mirror initiator the mirror manager creates a 
		mirror manager thread that will handle the request. the thread id and the informantion
		needed by the manager are stored in a list*/
		mflag=0;
		while(read(cli_sock,buff,MESSAGESIZE)>0)
		{
			if(strcmp(buff,"eom")==0)break;//end of message
			if(strcmp(buff,"shutdown")==0)
			{
				exit_all=1;
				pthread_cond_broadcast(&empty);//wake up all worker threads to make them shut down
				break;
			}
			else
			{
				if(mflag==0)
				{
					manager_count=atoi(buff);
					mflag=1;
				}
				else
				{
					id++;//the id is used to identify the delay of its request
					thread_list_insert(&thread_list,buff,id);
				}
			}
		}
		if(exit_all==1){close(cli_sock);break;}
		pthread_mutex_lock(&gen_mutex);
		pthread_cond_wait(&all_done,&gen_mutex);//MirrorServer waits here until the transfer is completed
		sprintf(buff,"%d %.0f %.2f",files_transfered,bytes_transfered,(bytes_transfered/files_transfered));
		write(cli_sock,buff,MESSAGESIZE);
		bytes_transfered=0;
		manager_count=0;
		managers_exited=0;
		files_to_transfer=0;
		files_transfered=0;
		pthread_mutex_unlock(&gen_mutex);
		
	}
	close(cli_sock);
	free(dirname);
	for (i = 0; i < threadnum; i++)pthread_join(thread_array[i], NULL);//wait for workers to shutdown
	delete_thread_list(thread_list);
	free(thread_array);
	delete_queue(pool_queue);
	return 0;
}



/* the mirror manager asks for the LIST of files that the Content server sees.
when it gets the results it puts in the buffer queue the files that are asked
by the initiator(if they exist)*/
void *mirror_manager(void *arg)
{
	int sock,error,temp,flag=0,file_counter=0;
	struct sockaddr_in server;
	struct hostent* rem;
	char buff[MESSAGESIZE]={'\0'},tempbuff1[MESSAGESIZE]={'\0'},tempbuff2[MESSAGESIZE]={'\0'},*tok1=NULL,*tok2=NULL,*token1=NULL,*token2=NULL;
	struct thread_listnode* head=(struct thread_listnode*)arg;
	struct path_listnode* path_list=NULL,*temp_path_list=NULL;
	/*this list has the file paths that were matching from the results of LIST in the content server.
	it is used in order to release the content server and put the filepaths in the buffer after closing the connection
	with the content server*/
	if((error=pthread_detach(pthread_self())) != 0){fprintf(stderr,"pthread_detach %s\n",strerror(error));exit(1);}
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){perror("socket");exit(-1);}
	if((rem = gethostbyname(head->content_server_address)) == NULL){herror("gethostbyname");exit(-1);}
	server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(head->port);
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){perror("connect");exit(-1);}//connect to the content server to ask for its list of files
    sprintf(buff,"LIST %d %d",head->content_server_id,head->delay);
    //printf("mirror manager: %s\n",buff );
    write(sock,buff,MESSAGESIZE);
    while(read(sock,buff,MESSAGESIZE)>0)
    {
    	if(strcmp(buff,"eom")==0)break;
    	strcpy(tempbuff1,head->dirorfile_path);
    	strcpy(tempbuff2,buff);
    	tok1=tempbuff1;
    	tok2=tempbuff2;
    	/*tok 1 has the path of the file or directory asked by the initiator
    	tok2 has a result of LIST.if the paths match the result is added to the list
    	in order to be added to the buffer later*/
    	while ((token1 = strtok_r(tok1, "/", &tok1)))
    	{
    		token2 = strtok_r(tok2,"/",&tok2);
    		if(strcmp(token1,token2)!=0){flag=1;break;}
    	}
    	if(flag==0)
		{
			file_counter++;
			path_list_insert(&path_list,buff);
		}
    	else flag=0;
    }
    pthread_mutex_lock(&gen_mutex);
    files_to_transfer+=file_counter;
    pthread_mutex_unlock(&gen_mutex);
    temp_path_list=path_list;
    if(path_list==NULL)printf("the file/directory asked is not valid\n");
    while(path_list!=NULL)//here all the matching results are placed in the buffer queue
    {
    	pthread_mutex_lock(&mutex);
    	//while(pool_queue->count >= QUEUE_LENGTH||num_of_cons!=0)pthread_cond_wait(&full, &mutex);
    	while(pool_queue->count >= QUEUE_LENGTH)pthread_cond_wait(&full, &mutex);
    	temp=pool_queue->front;
    	pool_queue->front = (pool_queue->front+1)%QUEUE_LENGTH;
    	pool_queue->count++;
    	//num_of_prods++;
    	pthread_mutex_lock(&mutex_array[temp]);
    	pthread_mutex_unlock(&mutex);
    	strcpy(pool_queue->buff[temp].dirorfilename,path_list->path);
    	strcpy(pool_queue->buff[temp].content_server_address,head->content_server_address);
    	pool_queue->buff[temp].cs_port=head->port;
    	pool_queue->buff[temp].content_server_id=head->content_server_id;
    	pthread_mutex_unlock(&mutex_array[temp]);
    	pthread_cond_signal(&empty);
    	//pthread_mutex_lock(&mutex);
		//num_of_prods--;
		//if (num_of_prods == 0)pthread_cond_broadcast(&empty);
		//pthread_mutex_unlock(&mutex);
    	path_list=path_list->next;
    }
    delete_path_list(temp_path_list);
    path_list=NULL;
    close(sock);
    pthread_mutex_lock(&gen_mutex);
    managers_exited++;
    pthread_mutex_unlock(&gen_mutex);
}

/*the worker threads run in a while(1) loop and when there are elements in the shared buffer
they consume them and ask the Content server mentioned in them to deliver the file*/

void *worker(void *arg)
{
	char* dirname=(char*)arg;
	int temp,sock,fd,flag=0,count,temp_count,i,worker_exit=0,error;
	struct buffer_element elem;
	char buff[MESSAGESIZE]={'\0'},tempbuff[MESSAGESIZE]={'\0'},tempbuff2[MESSAGESIZE]={'\0'},path[MESSAGESIZE]={'\0'},transfer[MESSAGESIZE]={'\0'};
	struct hostent* rem;
	struct sockaddr_in server;
	//if((error=pthread_detach(pthread_self())) != 0){fprintf(stderr,"pthread_detach %s\n",strerror(error));exit(1);}
	while(1)
	{
		
		pthread_mutex_lock(&mutex);
		//while(pool_queue->count <= 0||num_of_prods!=0)
		while(pool_queue->count <= 0)
		{
			pthread_cond_wait(&empty, &mutex);
			if(exit_all==1)
				{
					
					worker_exit=1;
					pthread_mutex_unlock(&mutex);
					break;
				}
		}
		if(worker_exit==1)break;
		temp=pool_queue->back;
		pool_queue->back=(pool_queue->back+1)%QUEUE_LENGTH;
		pool_queue->count--;
		//num_of_cons++;
		pthread_mutex_lock(&mutex_array[temp]);
		pthread_mutex_unlock(&mutex);
		strcpy(elem.content_server_address,pool_queue->buff[temp].content_server_address);
		strcpy(elem.dirorfilename,pool_queue->buff[temp].dirorfilename);
		elem.content_server_id=pool_queue->buff[temp].content_server_id;
		elem.cs_port=pool_queue->buff[temp].cs_port;
		//pthread_mutex_lock(&mutex);
		//num_of_cons--;
		//if (num_of_cons == 0)pthread_cond_broadcast(&full);
		//pthread_mutex_unlock(&mutex);
		//printf("worker: %d %s\n",(int)pthread_self(),elem.dirorfilename);
		pthread_mutex_unlock(&mutex_array[temp]);
		pthread_cond_signal(&full);

		/********* fetch**************/
		if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){perror("socket");exit(-1);}
		if((rem = gethostbyname(elem.content_server_address)) == NULL){herror("gethostbyname");exit(-1);}
		server.sin_family = AF_INET;      
		memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
   		server.sin_port = htons(elem.cs_port);
    	if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){perror("connect");exit(-1);}
    	sprintf(buff,"%s%s",dirname,elem.dirorfilename);
    	strcpy(tempbuff,elem.dirorfilename);
    	sprintf(path,"%s/",dirname);
    	char *token1,*token2,*tok1=tempbuff;
    	token1 = strtok_r(tok1, "/", &tok1);
    	token2 =strtok_r(tok1, "/", &tok1);
    	/*the path of the file is tokenised to make sure all the directories that are in it are created 
    	in the directory that the mirror server saves the files*/
    	while(1)
    	{
    		if(token2==NULL)break;
    		else
    		{
    			sprintf(tempbuff2,"%s/%s",path,token1);
    			strcpy(path,tempbuff2);
    			mkdir(tempbuff2,0777);
    			token1=token2;
    			token2=strtok_r(tok1, "/", &tok1);
    		}
    	}
    	fd = open(buff,O_CREAT|O_WRONLY,0777);
    	sprintf(buff,"FETCH %s %d",elem.dirorfilename,elem.content_server_id);
    	write(sock,buff,MESSAGESIZE);//ask for the content server to fetch the file
    	temp_count=0;//count the bytes transfered
    	while((count=read(sock,transfer,MESSAGESIZE))>0)//read from the content server
    	{
    		temp_count+=count;
    		write(fd,transfer,count);//write to the file created in the mirror server directory
    		for (i = 0; i < MESSAGESIZE; ++i)transfer[i]='\0';
    	}
    	pthread_mutex_lock(&gen_mutex);
    	bytes_transfered+=temp_count;
    	files_transfered++;
    	/*if all managers have exited it means */
    	if(managers_exited==manager_count && files_transfered==files_to_transfer)pthread_cond_signal(&all_done);
    	pthread_mutex_unlock(&gen_mutex);
    	close(sock);
    	close(fd);
	}
	pthread_exit(NULL);
}