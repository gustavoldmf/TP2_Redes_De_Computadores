all:
	gcc -g -Wall -c common.c
	gcc -g -Wall client.c common.o -o client
	gcc -g -Wall server.c common.o -o server
	gcc -g -Wall server-mt.c common.o -lpthread -o server-mt

clean:
	rm common.o client server server-mt 