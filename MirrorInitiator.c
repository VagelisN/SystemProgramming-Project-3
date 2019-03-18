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
#define MESSAGESIZE 512

int main(int argc, char **argv)
{
	int port, sock,i,count=1;
	char c,*serv_addr=NULL,*commands=NULL,*token=NULL,*cpt,buff[MESSAGESIZE]={'\0'};
	struct hostent* rem;
	struct sockaddr_in server;
	if(argc!=7){printf("To run: MirrorInitiator -n <MirrorServerAddress> -p <MirrorServerPort> -s <commands>\n");exit(1);}
	while((c = getopt (argc, argv, "n:p:s:")) != -1)
	{
		switch(c)
		{
			case 'n':
				serv_addr=malloc(sizeof(char)*strlen(optarg)+1);
				strcpy(serv_addr,optarg);
				break;
			case 'p':
				port=atoi(optarg);
				break;
			case 's':
				commands=malloc(sizeof(char)*MESSAGESIZE);
				for (i = 0; i < MESSAGESIZE; i++)commands[i]='\0';
				strcpy(commands,optarg);
				break;
			default:
				break;
		}		
	}
	/*Connects to the mirror server and sends the commands one by one through the socket*/
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){perror("socket");exit(-1);}
	if((rem = gethostbyname(serv_addr)) == NULL){herror("gethostbyname");exit(-1);}
	server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);
    if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){perror("connect");exit(-1);}
    if(strcmp(commands,"shutdown")!=0)
    {
    	cpt=commands;
		while(*cpt!='\0')
		{
			if(*cpt==',')count++;
			cpt++;
		}
		sprintf(buff,"%d",count);
		write(sock,buff,MESSAGESIZE);
	    token=strtok(commands,",");
	    write(sock,commands,MESSAGESIZE);
	    while((token=strtok(NULL,","))!=NULL)write(sock,token,MESSAGESIZE);
	    write(sock,"eom",MESSAGESIZE);
	    printf("Waiting for results...\n");
	    read(sock,buff,MESSAGESIZE);
	   	token=strtok(buff," ");
	   	printf("Files Transfered: %s\n",token);
	   	token=strtok(NULL," ");
	   	printf("Bytes Transfered: %s\n",token);
	   	token=strtok(NULL," ");
	   	printf("Average: %s\n",token);
	}
	else 
	{
		write(sock,commands,MESSAGESIZE);
	}
    close(sock);
    free(commands);
    free(serv_addr);
	return 0;
}