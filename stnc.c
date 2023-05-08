//
// Created by Lara Abu Hamad on 08/05/2023.
//

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_handler(void *socket) {
    int sockfd = *(int *)socket;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            break;
        }
        printf("%s\n", buffer);
    }
    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s -c IP PORT (client) or %s -s PORT (server)\n", argv[0], argv[0]);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    if (strcmp(argv[1], "-s") == 0) {
        int port = atoi(argv[2]);
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            perror("bind");
            return 1;
        }

        if (listen(sockfd, 1) < 0) {
            perror("listen");
            return 1;
        }

        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("accept");
            return 1;
        }

        sockfd = client_fd;
    } else if (strcmp(argv[1], "-c") == 0) {
        int port = atoi(argv[3]);
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, argv[2], &server_addr.sin_addr) <= 0) {
            perror("inet_pton");
            return 1;
        }

        if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            return 1;
        }
    } else {
        printf("Invalid option. Usage: %s -c IP PORT (client) or %s -s PORT (server)\n", argv[0], argv[0]);
        return 1;
    }
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_handler, &sockfd) < 0) {
        perror("pthread_create");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        ssize_t n = send(sockfd, buffer, strlen(buffer), 0);
        if (n <= 0) {
            break;
        }
    }

    close(sockfd);
    pthread_join(recv_thread, NULL);

    return 0;
}
