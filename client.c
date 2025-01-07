#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct Sensor_message {
    char type[20];
    int coords[2];
    float measurement;
}typedef Sensor_message;

//variaveis globais
Sensor_message sensor;

void usage(int argc, char **argv) {
	printf("Usage: ./client <server_ip> <port> -type <temperature|humidity|air_quality> -coords <x> <y>");
	exit(EXIT_FAILURE);
}

float atualiza_medicao( float medicao_remota, int *cord_s_viz) {
    // Calcula a distância entre as coordenadas
    int dx = sensor.coords[0] - cord_s_viz[0];
    int dy = sensor.coords[1] - cord_s_viz[1];
	double c = (dx * dx) + (dy * dy);
    double d = c; /// ATENÇAO AQUII ERROOOO !!!!!!!!!

    // Atualiza a medição
    float fator = 0.1 * (1.0 / (d + 1.0)); 
    float nova_medicao = sensor.measurement + fator * (medicao_remota - sensor.measurement);
	sensor.measurement = nova_medicao;

    return nova_medicao;
}

void * monitora_atualizacao_externa(void * p_client_socket){

	Sensor_message dado_local;
	int client_socket = *((int *)p_client_socket);
	while(1){

		if(recv(client_socket, &dado_local, sizeof(dado_local),0) == -1){
			logexit("Erro ao monitorar atualizacões");
		}
		else{
			atualiza_medicao(dado_local.measurement, dado_local.coords);
			printf("\nATUALIZACAO RECEBIDA, ATUALIZADA MAS NAO ENVIADA\n");
			printf("\nSensor");
			printf("%s\n", sensor.type);
			printf("coodenadas: <%d,%d>\n", sensor.coords[0], sensor.coords[1]);
			printf("valor: %f\n\n", sensor.measurement);

		}

	}

	return NULL;
}

#define BUFSZ 1024

int main(int argc, char **argv) {
	if (argc < 8) {
		usage(argc, argv);
	}

	srand(time(NULL));
	int random = 20 + rand() % ( 40 - 20 + 1); // atribui um numero aleatório entre 20 e 40 para o sensor de temperatura

////// PREENCHE INFORMAÇÕES DO SENSOR ////////
	strncpy(sensor.type, argv[4], sizeof(sensor.type) -1);
	sensor.coords[0] = atoi(argv[6]);
	sensor.coords[1] = atoi(argv[7]);
	sensor.measurement = random;
//////////////////////////////////////////////

///// IMPRIME OS DADOS DO SENSOR PARA CONFERIR /////

		printf("\nVALORES INICIAIS DESTE SENSOR:\n");
		printf("\nSensor");
		printf("%s\n", sensor.type);
		printf("coodenadas: <%d,%d>\n", sensor.coords[0], sensor.coords[1]);
		printf("valor: %f\n\n", sensor.measurement);
///////////////////////////////////////////////

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
    char buf[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("Conecção de sucesso com o servidor %s\n", addrstr);

    size_t count = send(s, &sensor, sizeof(sensor),0);

	pthread_t monitora_at;
	int *socket_client = malloc(sizeof(int));
	*socket_client = s;
	pthread_create(&monitora_at, NULL, monitora_atualizacao_externa, socket_client);


//Preciso criar 2 threads aqui, uma para gerenciar
// o tempo e fazer o envio de atualização das medidas
// e outro para monitorar se chegou a atualização de algum
// outro sensor do msm tipo

	while(1){

		printf("\n SLAOQ \n");
		sleep(2);

		if(strcmp(sensor.type, "temperature") == 0){
		printf("REFRESH ENVIADO temperatura");
		sleep(5);
		}
		else if(strcmp(sensor.type, "Humidade") == 0){
		printf("REFRESH ENVIADO humidade");
		sleep(7);
		}
		else if(strcmp(sensor.type, "Qualidade do Ar") == 0){
		printf("REFRESH ENVIADO qualidade");
		sleep(10);
		
		}

		size_t count = send(s, &sensor, sizeof(sensor),0);

		printf("\ncount = %ld\n", count);

	}
	
	close(s);
	exit(EXIT_SUCCESS);
}