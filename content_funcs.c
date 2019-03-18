#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>		
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include "content_funcs.h"
#define MESSAGESIZE 512

int id_list_insert(struct id_listnode **head,int id,int delay)
{
	if((*head)==NULL)
	{
		(*head)=malloc(sizeof(struct id_listnode));
		(*head)->id=id;
		(*head)->delay=delay;
		(*head)->next=NULL;
	}
	else
	{
		struct id_listnode *temp=(*head);
		while((*head)->next!=NULL)(*head)=(*head)->next;
		(*head)->next=malloc(sizeof(struct id_listnode));
		(*head)=(*head)->next;
		(*head)->id=id;
		(*head)->delay=delay;
		(*head)->next=NULL;
		(*head)=temp;
	}
	return 0;
}
void id_list_delete(struct id_listnode* head)
{
	struct id_listnode *temp;
	while(head!=NULL)
	{
		temp=head;
		head=head->next;
		free(temp);
	}
}


void content_serve(int cli_sock,char* dirname,char* buff)
{
	char *token=NULL;
	token=strtok(buff," ");
	char transfer[MESSAGESIZE];
	if(strcmp(token,"LIST")==0)
	{
		dir_content_list(dirname,cli_sock,dirname);//call dir content list to find recursively the contents of the folder shared by the content server
		write(cli_sock,"eom",MESSAGESIZE);
	}
	else if(strcmp(token,"FETCH")==0)//just sleeps for <delay> seconds and fetches the file from the socket 
	{
		int fd,delay,count;
		char path[MESSAGESIZE]={'\0'};
		token=strtok(NULL," ");
		delay=atoi(strtok(NULL," "));
		sprintf(path,"%s%s",dirname,token);
		fd=open(path,O_RDONLY);
		sleep(delay);
		while((count=read(fd,transfer,MESSAGESIZE))>0)
		{
			write(cli_sock,transfer,count);
			int i;
			for ( i = 0; i < MESSAGESIZE; ++i)transfer[i]='\0';
		}
		close(fd);
	}
}

void dir_content_list(char* dirname, int sock,char* starting_path)//recursively finds the contents of dirname,the starting path is used to be cut from the begining of each path*/
{
	char buff[512],*token=NULL;
	DIR *dir;
	struct dirent *st_dirent;
	if((dir = opendir(dirname)) == NULL ){perror("opendir");exit(2);}
	while(st_dirent = readdir(dir))
	{
		if(st_dirent->d_type == 4)
		{
			if (strcmp(st_dirent->d_name, ".") != 0 && strcmp(st_dirent->d_name, "..") != 0)//ignore parent and current
			{	
				sprintf(buff,"%s/%s",dirname,st_dirent->d_name);
				dir_content_list(buff,sock,starting_path);
			}
		}
		else
		{
			sprintf(buff,"%s/%s",dirname,st_dirent->d_name);
			token=buff;
			token=token+strlen(starting_path);
			write(sock,token,MESSAGESIZE);//for each file found write it in the socket
		}
	}
	closedir(dir);
}
