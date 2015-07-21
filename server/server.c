#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"
#include "utility.h"

int main(int argc, char * argv[]) {
    int *tid_ptr;
    pthread_t tid;
    struct sockaddr_in serv_addr;

    if(argc < 2) {
        printf("Usage: ./server_exec <port number>.\n");
		exit(1);
    }

    listen_fd = my_socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((short)atoi(argv[1]));

    my_bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    unsigned int len = sizeof(serv_addr);
    getsockname(listen_fd, (struct sockaddr *)&serv_addr, &len);

    printf("\nListening on port %s...\n", argv[1]);

    my_listen(listen_fd, 5);

    for (int i = 0; i < CLIENT_SIZE; ++i) {     // Setup client data
        client[i].cli_sock = -1;                // -1 indicates available entry
        client[i].is_quiet = false;
    }

    for (int i = 0; i < CLIENT_SIZE; ++i) {
        tid_ptr = (int*)malloc(sizeof(int));
        *tid_ptr = i;
        pthread_create(&tid, NULL, &start_client, (void*)tid_ptr);
    }

    for (;;)
        pause();
}




