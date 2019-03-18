#ifndef MIRROR_FUNCS_H
#define MIRROR_FUNCS_H
#define QUEUE_LENGTH 10


struct thread_listnode
{
	char* content_server_address;
	int port;
	char* dirorfile_path;
	int delay; 
	pthread_t thr;
	int content_server_id;
	struct thread_listnode* next;
};

struct path_listnode
{
	char* path;
	struct path_listnode* next;
};


struct buffer_element
{
	char dirorfilename[512];
	char content_server_address[512];
	int cs_port;
	int content_server_id;
	
};

struct queue
{
	struct buffer_element buff[QUEUE_LENGTH];
	int front;
	int back;
	int count;
};

void *mirror_manager(void *);
void *worker(void *);

int thread_list_insert(struct thread_listnode**,char*,int);
void delete_thread_list(struct thread_listnode*);

int path_list_insert(struct path_listnode**,char*);
void delete_path_list(struct path_listnode*);

int queue_init(struct queue**);
void delete_queue(struct queue*);



#endif