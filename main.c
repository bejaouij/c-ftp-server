#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define PORT 32456

#define CLIENTS_MAX_COUNT 20
#define COMMAND_BUFFER_MAX_LENGTH 256
#define MESSAGE_BUFFER_MAX_LENGTH 2048

#define FILE_LISTING_COMMAND "ls -A"

typedef struct Client {
	int index;
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
	pthread_t communicationThread;
} Client;

typedef struct Gateway {
	int socketDescriptor;
	int clientsCount;
	Client* clients[CLIENTS_MAX_COUNT];
} Gateway;

typedef struct MessageTransmissionParams {
    Client* senderClient;
    Gateway* gateway;
} MessageTransmissionParams;

void* t_communication();

void _executeCommand(char* cmd, char* buffer);

int main() {
	/* Initialize the server side socket */
	Gateway gateway;
	gateway.clientsCount = 0;
	gateway.socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);

	if(gateway.socketDescriptor == -1) {
		perror("Socket Creation Error");
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if(bind(gateway.socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) == -1) {
		perror("Socket Binding Error");
	}
	/********************/

	/* Make server side socket listen connection request */
	if(listen(gateway.socketDescriptor, 2) == -1) {
		perror("Socket Linstening Error");
	} else {
        printf("Server started on port %i\n", PORT);
	}
	/********************/

    /* Make the gateway between both clients to make the communication works */
    int i;
    int resThreadCreation;
    MessageTransmissionParams* communicationParams;

    while(1) {
        if(gateway.clientsCount != CLIENTS_MAX_COUNT) {
            i = gateway.clientsCount;
            gateway.clients[i] = (Client*) malloc(sizeof(Client));
            gateway.clients[i]->index = i;
            gateway.clients[i]->addressLength = sizeof(struct sockaddr_in);

            if((gateway.clients[i]->socketDescriptor = accept(gateway.socketDescriptor, (struct sockaddr*)&(gateway.clients[i]->address), &(gateway.clients[i]->addressLength))) == -1) {
                perror("Client Socket Accepting Error");
                return -1;
            }

            printf("New connection\n");

            communicationParams = (MessageTransmissionParams*) malloc(sizeof(MessageTransmissionParams));
            communicationParams->senderClient = gateway.clients[i];
            communicationParams->gateway = &gateway;

            resThreadCreation = pthread_create(&gateway.clients[i]->communicationThread, NULL, (void*) &t_communication, communicationParams);

            if(resThreadCreation != 0) {
                perror("Thread Creation Error");
            }

            gateway.clientsCount++;
            /********************/
        }
    }
    /********************/

    /* Close the server side socket */
    if(close(gateway.socketDescriptor) == -1) {
        perror("Socket Closing Error");
    }
    /********************/

	return 0;
}

void* t_communication(MessageTransmissionParams* params) {
    int resSend, resRecv;
    int isGatewayEstablished = 1;
    char commandBuffer[COMMAND_BUFFER_MAX_LENGTH];
    char messageBuffer[MESSAGE_BUFFER_MAX_LENGTH];

    while(isGatewayEstablished == 1) {
        if((resRecv = recv(params->senderClient->socketDescriptor, &commandBuffer, sizeof(char)*COMMAND_BUFFER_MAX_LENGTH, 0)) == -1) {
            perror("Command Reception Error");
            isGatewayEstablished = 0;
        }
        else if(resRecv == 0) {
            perror("Client Down");
            isGatewayEstablished = 0;
        }
        else {
            if(strcmp(commandBuffer, "rls") == 0) {
                char* cmd = FILE_LISTING_COMMAND;

                _executeCommand(cmd, commandBuffer);
            }

            strcpy(messageBuffer, commandBuffer);

            if((resSend = send(params->senderClient->socketDescriptor, &messageBuffer, (MESSAGE_BUFFER_MAX_LENGTH*sizeof(char)), 0)) == -1) {
                perror("Nickname Feedback Sending Error");
                isGatewayEstablished = 0;
            }
            if(resSend == 0) {
                perror("Client Down");
                isGatewayEstablished = 0;
            }
        }
    }
}

void _executeCommand(char* cmd, char* buffer) {
    FILE *pFile;

    if((pFile = popen(cmd, "r")) == NULL) {
        perror("Pipe Opening Error");
        return;
    }

    char filename[COMMAND_BUFFER_MAX_LENGTH];

    while(fgets(filename, COMMAND_BUFFER_MAX_LENGTH, pFile) != NULL) {
        strcat(buffer, filename);
    }

    if(pclose(pFile))  {
        perror("Pipe Closing Error");
        return;
    }
}

