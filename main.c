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

typedef struct Client {
	int index;
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
} Client;

typedef struct Gateway {
	int socketDescriptor;
	int clientsCount;
	Client* clients[CLIENTS_MAX_COUNT];
} Gateway;

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

            /* Useful in the case where clients count value
             * has been changed during the accepting process. */
            gateway.clients[gateway.clientsCount] = gateway.clients[i];
            i = gateway.clientsCount;
            gateway.clients[i]->index = i;
            /********************/

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

