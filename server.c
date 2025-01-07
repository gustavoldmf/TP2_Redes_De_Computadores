#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_BACKLOG 100
#define BUFSZ 1024
#define MAX_SENSOR 12 - 9

struct Sensor_message {
    char type[20];
    int coords[2];
    float measurement;
}typedef Sensor_message;

//variaveis globais do servidor
Sensor_message all_sensors[MAX_SENSOR];
int s_index = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// tem a função de redirecionar as atualizações para
// cada sensor do mesmo tipo 
void * gerencia_sensores(void * p_cliente_socket){

    char buf[BUFSZ];
    int client_socket = *((int*)p_cliente_socket);
    free(p_cliente_socket);
    Sensor_message dados;

    if(recv(client_socket, &dados, sizeof(dados)+1,0) == -1){
			logexit("Erro ao monitorar atualizacões");
	}
    int n_cliente = client_socket - 4;

    // printf("Este é o cliente numero %d",n_cliente);

    pthread_mutex_lock(&mutex); // apenas esta thread podera alterar neste momento
    if(s_index < MAX_SENSOR){
        all_sensors[s_index] = dados;
        s_index++;
    }
    pthread_mutex_unlock(&mutex); // libera para que outra thread possa fazer alterações

    for(int i = 0; i < MAX_SENSOR ; i++){
        printf("\nSensor numero %d\n", i);
        printf("%s\n", all_sensors[i].type);
        printf("coodenadas: <%d,%d>\n", all_sensors[i].coords[0], all_sensors[i].coords[1]);
        printf("valor: %f\n\n", all_sensors[i].measurement);
    }    

    while(1){

        if(recv(client_socket, &dados, sizeof(dados),0) == -1){
            logexit("atualização temporaria");
        }
        else{

            printf("RECEBIDO");

            pthread_mutex_lock(&mutex); // apenas esta thread podera alterar neste momento

            all_sensors[client_socket-4] = dados;
            pthread_mutex_unlock(&mutex); // libera para que outra thread possa fazer alterações

            for(int i = 0; i < MAX_SENSOR; i++){
                if(strcmp(all_sensors[i].type, "temperature")){
                    send(client_socket+i+4, &dados, sizeof(dados), 0);
                }
            }
            // Como enviar esses dados para os outros clientes????????

            }
        }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, SERVER_BACKLOG)) {
        logexit("listen");
    }

    for(int j = 0; j < MAX_SENSOR; j++){
        memset(all_sensors, 0, sizeof(all_sensors));
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock;
    


        while(1){

            csock = accept(s, caddr, &caddrlen);
            if (csock == -1) {
                logexit("accept");
            }

            printf("\nCliente aceito com csock = %d\n\n", csock);
            pthread_t t; 

            int *pclient = malloc(sizeof(int));
            *pclient = csock;
            pthread_create(&t, NULL, gerencia_sensores, pclient);

        }

        close(csock);

    exit(EXIT_SUCCESS);
}