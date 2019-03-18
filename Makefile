all: MirrorServer MirrorInitiator ContentServer

MirrorServer: MirrorServer.o mirror_funcs.o
	gcc -g3 MirrorServer.o mirror_funcs.o -o MirrorServer -lpthread

MirrorInitiator: MirrorInitiator.o
	gcc -g3 MirrorInitiator.o -o MirrorInitiator

ContentServer: ContentServer.o content_funcs.o
	gcc -g3 ContentServer.o content_funcs.o -o ContentServer

MirrorServer.o: MirrorServer.c
	gcc -c MirrorServer.c

mirror_funcs.o: mirror_funcs.c mirror_funcs.h
	gcc -c mirror_funcs.c

MirrorInitiator.o: MirrorInitiator.c
	gcc -c MirrorInitiator.c

ContentServer.o: ContentServer.c
	gcc -c ContentServer.c

content_funcs.o: content_funcs.c content_funcs.h
	gcc -c content_funcs.c

clean:
	rm *.o MirrorServer MirrorInitiator ContentServer
