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
Sensor_message all_sensors_aux[MAX_SENSOR];
int s_index = 0;
int count  = 0;
int flag_ja_atual = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

//Função que verifica se ocorreu alguma atualização de algum outro cliente. 
int Check_Atualization(void){
    for (int i = 0; i < MAX_SENSOR; i++)
    {
        if(all_sensors[i].measurement != all_sensors_aux[i].measurement){ //se sim, significa que ocoreu atualização
            return i; // retorna qual dos clientes atualizou, assim eu consigo puxar seu tipo.
        }
    }
    return 0;
}

// void * teste(void * p_client_socket){

//     char teste_buf[BUFSZ];
//     int teste_client_socket = *((int*)p_client_socket);
//     int teste_number_client = teste_client_socket - 4;
//     free(p_client_socket);
//     Sensor_message teste_dados;

//     while(1){
//         re
//     }

// }

// Faz a logica de troca de dados com o cliente 
void * gerencia_sensores(void * p_cliente_socket){

    char buf[BUFSZ];
    int client_socket = *((int*)p_cliente_socket);
    int number_client = client_socket - 4;
    printf("\n\nESTE É O SENSOR: %d", number_client);
    free(p_cliente_socket);
    Sensor_message dados;
    int check;

    //supondo que ele passe aqui apenas uma vez

    while(count < 100){
    count++;

    check = Check_Atualization();
    if( check != 0 && flag_ja_atual){ // se existe atulização ent:
        flag_ja_atual = 0; 
        if (strcmp(all_sensors[check].type , all_sensors[number_client].type) == 0) // se o cliente que está conversando é do mesmo tipo que o cliente que atualizou, o servidor usa um send para enviar a atualização para recalibragem.
        {
            printf("\nESPERANDO PARA ENVIAR para Sensor %d\n", number_client);
            send(client_socket, &all_sensors[check], sizeof(all_sensors[check]), 0);
            printf("\nENVIADO para Sensor %d\n", number_client);
        } 
    }
    else{
    
    printf("\nESPERANDO RECEBER do Sensor %d\n", number_client);
    if(recv(client_socket, &dados, sizeof(dados)+1,0) == -1){
			logexit("Monitoramento de atualizacoes");
	} 
     printf("RECEBIDO do Sensor %d", number_client);

    pthread_mutex_lock(&mutex); // apenas esta thread podera alterar neste momento
    all_sensors_aux [number_client] = all_sensors[number_client]; // copia o antigo valor daquele sensor na struct auxiliar
    all_sensors[number_client] = dados;
    flag_ja_atual = 1;
    pthread_mutex_unlock(&mutex); // libera para que outra thread possa fazer alterações 

    printf("\n////////////////// ATUALIZACAO SENSOR %d ///////////////////////\n", number_client);
    for(int i = 0; i < MAX_SENSOR ; i++){
        printf("\nSensor numero %d\n", i);
        printf("%s\n", all_sensors[i].type);
        printf("coodenadas: <%d,%d>\n", all_sensors[i].coords[0], all_sensors[i].coords[1]);
        printf("valor: %f\n\n", all_sensors[i].measurement);
    }    
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

            printf("\nSensor aceito com csock = %d\n\n", csock - 4);
            pthread_t function1;


            int *pclient = malloc(sizeof(int));
            *pclient = csock;
            pthread_create(&function1, NULL, gerencia_sensores, pclient);


        }

        close(csock);

    exit(EXIT_SUCCESS);
}