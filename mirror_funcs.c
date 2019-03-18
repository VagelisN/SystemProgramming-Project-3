#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "mirror_funcs.h"
#define MESSAGESIZE 512

//just list functions 

int thread_list_insert(struct thread_listnode **head,char buff[MESSAGESIZE],int id)
{
	int error;
	char* token=NULL;
	if((*head)==NULL)
	{
		(*head)=malloc(sizeof(struct thread_listnode));
		token=strtok(buff,":");
		(*head)->content_server_address=malloc(sizeof(char)*strlen(token)+1);
		strcpy((*head)->content_server_address,token);
		token=strtok(NULL,":");
		(*head)->port=atoi(token);
		token=strtok(NULL,":");
		(*head)->dirorfile_path=malloc(sizeof(char)*strlen(token)+1);
		strcpy((*head)->dirorfile_path,token);
		token=strtok(NULL,":");
		(*head)->delay=atoi(token);
		(*head)->content_server_id=id;
		if(error=pthread_create(&(*head)->thr,NULL,mirror_manager,(void*)(*head))){fprintf(stderr,"pthread_create %s\n",strerror(error));exit(1);}
		(*head)->next=NULL;
	}
	else
	{
		struct thread_listnode *temp=(*head);
		while((*head)->next!=NULL)(*head)=(*head)->next;
		(*head)->next=malloc(sizeof(struct thread_listnode));
		(*head)=(*head)->next;
		token=strtok(buff,":");
		(*head)->content_server_address=malloc(sizeof(char)*strlen(token)+1);
		strcpy((*head)->content_server_address,token);
		token=strtok(NULL,":");
		(*head)->port=atoi(token);
		token=strtok(NULL,":");
		(*head)->dirorfile_path=malloc(sizeof(char)*strlen(token)+1);
		strcpy((*head)->dirorfile_path,token);
		token=strtok(NULL,":");
		(*head)->delay=atoi(token);
		(*head)->content_server_id=id;
		if(error=pthread_create(&(*head)->thr,NULL,mirror_manager,(void*)(*head))){fprintf(stderr,"pthread_create %s\n",strerror(error));exit(1);}
		(*head)->next=NULL;
		(*head)=temp;
	}
	return 0;
}

void delete_thread_list(struct thread_listnode* head)
{
	struct thread_listnode *temp;
	while(head!=NULL)
	{
		temp=head;
		head=head->next;
		free(temp->content_server_address);
		free(temp->dirorfile_path);
		free(temp);
	}
}

int path_list_insert(struct path_listnode **head,char* cur_path)
{
	if((*head)==NULL)
	{
		(*head)=malloc(sizeof(struct path_listnode));
		(*head)->path=malloc(sizeof(char)*strlen(cur_path)+1);
		strcpy((*head)->path,cur_path);
		(*head)->next=NULL;
	}
	else
	{
		struct path_listnode *temp=(*head);
		while((*head)->next!=NULL)(*head)=(*head)->next;
		(*head)->next=malloc(sizeof(struct path_listnode));
		(*head)=(*head)->next;
		(*head)->path=malloc(sizeof(char)*strlen(cur_path)+1);
		strcpy((*head)->path,cur_path);
		(*head)->next=NULL;
		(*head)=temp;
	}
	return 0;
}

void delete_path_list(struct path_listnode *head)
{
	struct path_listnode *temp;
	while(head!=NULL)
	{
		temp=head;
		head=head->next;
		free(temp->path);
		free(temp);
	}
}

int queue_init(struct queue** cur_queue)
{
	int i;
	(*cur_queue)=malloc(sizeof(struct queue));
	(*cur_queue)->front=0;
	(*cur_queue)->back=0;
	(*cur_queue)->count=0;
}

void delete_queue(struct queue* pool)
{
	free(pool);
}

