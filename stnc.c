#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFFER 1024

void handle_client(int client_sock) {
    char buffer[MAX_BUFFER];
    fd_set readfds;
    int max_fd;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(client_sock, &readfds);
        max_fd = (STDIN_FILENO > client_sock) ? STDIN_FILENO : client_sock;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(buffer, MAX_BUFFER, stdin);
            send(client_sock, buffer, strlen(buffer), 0);
        }

        if (FD_ISSET(client_sock, &readfds)) {
            int nbytes = recv(client_sock, buffer, MAX_BUFFER - 1, 0);
            if (nbytes <= 0) {
                if (nbytes < 0) {
                    perror("recv");
                }
                close(client_sock);
                exit(EXIT_SUCCESS);
            }
            buffer[nbytes] = '\0';
            printf("Received: %s", buffer);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || (strcmp(argv[1], "-c") == 0 && argc != 4)) {
        fprintf(stderr, "Usage: %s -c IP PORT or %s -s PORT\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if (strcmp(argv[1], "-c") == 0) {
        addr.sin_port = htons(atoi(argv[3]));
        addr.sin_addr.s_addr = inet_addr(argv[2]);

        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(argv[1], "-s") == 0) {
        addr.sin_port = htons(atoi(argv[2]));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(sockfd, 1) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        int client_sock;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if ((client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        close(sockfd);
        sockfd = client_sock;
    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    handle_client(sockfd);
    close(sockfd);

    return 0;
}
