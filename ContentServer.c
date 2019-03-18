#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>		
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include "content_funcs.h"
#define MESSAGESIZE 512


int main(int argc, char **argv)
{
	int port,sock,cli_sock,temp_id,temp_delay;
	struct sockaddr_in ser,cli;
	struct id_listnode *id_list=NULL,*temp_list=NULL;
	char c,*dirname,buff[MESSAGESIZE],tempbuff[MESSAGESIZE],*token,*token2;
	socklen_t client_len;
	if(argc!=5){printf("To run: ContentServer -p <port> -d <dirname>\n");exit(1);}
	while((c = getopt (argc, argv, "p:d:")) != -1)
	{
		switch(c)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'd':
				dirname=malloc(sizeof(char) * (strlen(optarg)+1));
				strcpy(dirname,optarg);
				break;
			default:
				break;
		}		
	}
	if((sock=socket(PF_INET, SOCK_STREAM, 0)) ==-1){perror("socket");exit(-1);}
	ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    ser.sin_port = htons(port);
    if(bind(sock,(struct sockaddr*)&ser,sizeof(ser)) == -1){perror("bind");exit(-1);}
    if(listen(sock,5) == -1){perror("listen");exit(-1);}//listen for connections from either MirrorManagers or Workers
    while(1)
	{
		client_len=sizeof(cli);
		printf("Waiting for Connection..\n");
		if((cli_sock = accept(sock,(struct sockaddr*)&cli, &client_len)) ==-1){perror("accept");exit(-1);}
		read(cli_sock,buff,MESSAGESIZE);
		strcpy(tempbuff,buff);
		token=strtok(tempbuff," ");
		/*before calling fork the command set is tokenised in order to save in a list the id of the call*/
		if(strcmp(token,"LIST")==0)
		{
			token=strtok(NULL," ");
			temp_id=atoi(token);
			token=strtok(NULL," ");
			temp_delay=atoi(token);
			id_list_insert(&id_list,temp_id,temp_delay);
		}
		/*having saved the id of the command in LIST the content server
		 knows the number of seconds to sleep before sending the file*/
		else if(strcmp(token,"FETCH")==0)
		{
			token2=strtok(NULL," ");
			token=strtok(NULL," ");
			temp_id=atoi(token);
			temp_list=id_list;
			while(temp_list!=NULL)
			{
				if(temp_list->id==temp_id)break;
				temp_list=temp_list->next;
			}
			sprintf(buff,"FETCH %s %d",token2,temp_list->delay);
		}
		if(fork()==0)//child
		{
			close(sock);
			content_serve(cli_sock,dirname,buff);
			close(cli_sock);
			exit(0);
		}
		else//parent
		{
			close(cli_sock);
		}
	}
	id_list_delete(id_list);
	return 0;
}



