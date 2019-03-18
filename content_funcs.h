#ifndef CONTENT_FUNCS_H
#define CONTENT_FUNCS_H

struct id_listnode
{
	int id;
	int delay;
	struct id_listnode* next;
};

int id_list_insert(struct id_listnode**,int,int);
void id_list_delete(struct id_listnode*);

void content_serve(int,char*,char*);
void dir_content_list(char*, int,char*);

#endif