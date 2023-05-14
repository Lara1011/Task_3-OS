//
// Created by malaklinux on 5/9/23.
//

#ifndef TASK_3_OS_STNC_H
#define TASK_3_OS_STNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/un.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <openssl/sha.h>

#define CLIENT_SOCK_FILE "client.sock"
#define SERVER_SOCK_FILE "server.sock"

// The maximum size of data that can be sent at once
#define MAX_BUFFER 1024

// The size of the chunk of data for the performance test
#define CHUNK_SIZE (100 * MAX_BUFFER * MAX_BUFFER) // 100MB

typedef enum {
    IPV4_TCP,
    IPV4_UDP,
    IPV6_TCP,
    IPV6_UDP,
    UDS_DGRAM,
    UDS_STREAM,
    MMAP,
    PIPE,
} CommunicationType;

typedef struct {
    char *ip;
    long port;
    int socket_fd;
    CommunicationType type;
    char *filename; // For mmap and pipe
    int mmap_fd; // For mmap
    char *mmap_addr; // For mmap
    int pipe_fd[2]; // For pipe
    struct sockaddr *addr;
    socklen_t addr_len;
    size_t mmap_size;
} Connection;

typedef struct {
    char *data_chunk;
    char *data_hash;
    long  data_length;
    long long transmission_time;
} PerformanceTest;

void handle_client_pA(int client_sock);

void generate_data(PerformanceTest *test);

void calculate_hash(PerformanceTest *test);

void transmit_data(Connection *connection, PerformanceTest *test);

PerformanceTest *create_performance_test();

void report_result(PerformanceTest *test);

Connection *create_connection(char *ip, long port, CommunicationType type, char *filename, int is_server);

Connection *open_connection(char *ip, long port, char *type, char *param, int is_server);

int create_tcp_server_socket(char *ip, long port, int domain);
int create_udp_server_socket(char *ip, long port, int domain);
int create_uds_server_socket(char *filename, int is_stream);

int create_tcp_client_socket(char *ip, long port, int domain);
int create_udp_client_socket(char *ip, long port, int domain);
int create_uds_client_socket(char *filename, int is_stream);

int create_mmap_file(Connection *connection);
int create_mmap_filer(Connection *connection);

int create_named_pipe(Connection *connection);
int create_named_pipew(Connection *connection);

void destroy_connection(Connection *connection);

long send_data(Connection *connection, char *message, int len);

void handle_client_pB(Connection *conn);

int main(int argc, char *argv[]);

#endif //TASK_3_OS_STNC_H