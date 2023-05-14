
//
// Created by Lara Abu Hamad on 5/11/23.
//

#include <netdb.h>
#include <ifaddrs.h>
#include "stnc.h"

//<type> ipv4,ipv6,mmap,pipe,uds
//<param> udp, tcp, dgram, stream, file name:

//------------------------------------------------------------------------------------------
//------------------------------P A R T   A-------------------------------------------------
//------------------------------------------------------------------------------------------
void handle_client_pA(int client_sock) {
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
            //fgets(buffer, MAX_BUFFER, stdin);
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

//------------------------------------------------------------------------------------------
//------------------------------P A R T   B-------------------------------------------------
//------------------------------------------------------------------------------------------

void generate_data(PerformanceTest *test) {
    srand(time(NULL));
    for (size_t i = 0; i < CHUNK_SIZE; i++) {
        test->data_chunk[i] = (char) (rand() % 256);
    }
}

void calculate_hash(PerformanceTest *test) {
    unsigned long long hash = 0;
    for (size_t i = 0; i < CHUNK_SIZE; i++) {
        hash += test->data_chunk[i];
    }

// Allocate memory for the hash string
    test->data_hash = malloc(sizeof(char) * 21); // Enough for 64-bit number
    if (test->data_hash == NULL) {
        fprintf(stderr, "Failed to allocate memory for data hash\n");
        return;
    }

    sprintf(test->data_hash, "%llu", hash);
}

void transmit_data(Connection *connection, PerformanceTest *test) {
    clock_t start, end;
    start = clock();
    printf("transmit_data %d\n", connection->type);
    send_data(connection, test->data_chunk, test->data_length);

    end = clock();
    test->transmission_time = (long long) (end - start) * 1000 / CLOCKS_PER_SEC; // convert to milliseconds
}

PerformanceTest *create_performance_test() {
    PerformanceTest *test = malloc(sizeof(PerformanceTest));
    if (!test) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Initialize the data chunk with a size of CHUNK_SIZE
    test->data_chunk = malloc(CHUNK_SIZE);
    if (!test->data_chunk) {
        perror("malloc");
        free(test);
        exit(EXIT_FAILURE);
    }

    // Fill the data chunk with random data
    generate_data(test);
    test->data_length = CHUNK_SIZE;
    // Initialize the data hash with a size of enough to hold the hash value
    // Assuming we are using SHA-256, so we need 32 bytes for the hash value
    test->data_hash = malloc(SHA256_DIGEST_LENGTH);
    if (!test->data_hash) {
        perror("malloc");
        free(test->data_chunk);
        free(test);
        exit(EXIT_FAILURE);
    }

    // Calculate the hash for the data chunk
    calculate_hash(test);

    // Initialize the transmission time with zero
    test->transmission_time = 0;

    return test;
}

void report_result(PerformanceTest *test) {
    printf("Hash: %s, Time: %lld ms\n", test->data_hash, test->transmission_time);
}

Connection *create_connection(char *ip, long port, CommunicationType type, char *filename, int is_server) {

    Connection *connection = malloc(sizeof(Connection) + 1);

    if (connection == NULL) {
        perror("Failed to allocate memory for Connection struct");
        exit(EXIT_FAILURE);
    }
    connection->ip = ip;
    connection->port = port;
    connection->type = type;
    connection->filename = filename;


    switch (type) {
        case IPV4_TCP:
            connection->socket_fd = is_server ? create_tcp_server_socket(ip, port, AF_INET)
                                              : create_tcp_client_socket(ip, port, AF_INET);
            if (connection->socket_fd == -1) {
                fprintf(stderr, "Failed to create IPV4 TCP socket.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case IPV4_UDP:
            connection->socket_fd = is_server ? create_udp_server_socket(ip, port, AF_INET)
                                              : create_udp_client_socket(ip, port, AF_INET);
            if (connection->socket_fd == -1) {
                fprintf(stderr, "Failed to create IPV4 UDP socket.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case IPV6_TCP:
            connection->socket_fd = is_server ? create_tcp_server_socket(ip, port, AF_INET6)
                                              : create_tcp_client_socket(ip, port, AF_INET6);
            if (connection->socket_fd == -1) {
                fprintf(stderr, "Failed to create IPV6 TCP socket.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case IPV6_UDP:
            connection->socket_fd = is_server ? create_udp_server_socket(ip, port, AF_INET6)
                                              : create_udp_client_socket(ip, port, AF_INET6);
            if (connection->socket_fd == -1) {
                fprintf(stderr, "Failed to create IPV6 UDP socket.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case UDS_DGRAM:
        case UDS_STREAM:
            connection->socket_fd = is_server ? create_uds_server_socket(filename, type == UDS_STREAM)
                                              : create_uds_client_socket(filename, type == UDS_STREAM);
            if (connection->socket_fd == -1) {
                fprintf(stderr, "Failed to create Unix Domain Socket.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case MMAP:
            if (is_server)
                connection->mmap_fd = create_mmap_file(connection);
            else
                connection->mmap_fd = create_mmap_filer(connection);
            if (connection->mmap_fd == -1) {
                fprintf(stderr, "Failed to create memory-mapped file.\n");
                free(connection);
                exit(EXIT_FAILURE);
            }
            break;
        case PIPE:
            if (is_server) {

                printf("sss1+\n");
                if (create_named_pipe(connection) == -1) {
                    fprintf(stderr, "Failed to create named pipe.\n");
                    free(connection);
                    exit(EXIT_FAILURE);
                }
                printf("sss1-\n");
            } else {
                printf("sss2+\n");
                if (create_named_pipew(connection) == -1) {
                    fprintf(stderr, "Failed to create named pipe.\n");
                    free(connection);
                    exit(EXIT_FAILURE);
                }
                printf("sss2-\n");

            }

            break;
        default:
            fprintf(stderr, "Unknown communication type.\n");
            free(connection);
            exit(EXIT_FAILURE);
    }

    return connection;
}

Connection *open_connection(char *ip, long port, char *type, char *param, int is_server) {
    CommunicationType comm_type;


    // Parse the type and param strings
    if (strcmp(type, "ipv4") == 0) {
        if (strcmp(param, "tcp") == 0) {
            comm_type = IPV4_TCP;
        } else if (strcmp(param, "udp") == 0) {
            comm_type = IPV4_UDP;
        } else {
            fprintf(stderr, "Invalid param for ipv4: %s\n", param);
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(type, "ipv6") == 0) {
        if (strcmp(param, "tcp") == 0) {
            comm_type = IPV6_TCP;
        } else if (strcmp(param, "udp") == 0) {
            comm_type = IPV6_UDP;
        } else {
            fprintf(stderr, "Invalid param for ipv6: %s\n", param);
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(type, "uds") == 0) {
        if (strcmp(param, "dgram") == 0) {
            comm_type = UDS_DGRAM;
        } else if (strcmp(param, "stream") == 0) {
            comm_type = UDS_STREAM;
        } else {
            fprintf(stderr, "Invalid param for uds: %s\n", param);
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(type, "mmap") == 0 || strcmp(type, "pipe") == 0) {
        comm_type = (strcmp(type, "mmap") == 0) ? MMAP : PIPE;

        // param is the filename for mmap and pipe
    } else {
        fprintf(stderr, "Invalid type: %s\n", type);
        exit(EXIT_FAILURE);
    }


    // Create a new Connection struct
    Connection *conn = create_connection(ip, port, comm_type, param, is_server);


    return conn;
}

int create_tcp_server_socket(char *ip, long port, int domain) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr *addr;
    int yes = 1;
    char port_str[6]; // 5 digits for port number + null terminator
    sprintf(port_str, "%ld", port);

// Create socket
    int socket_fd = socket(domain, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Allow the socket to be reused immediately after program exit
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

// Set up the server address structure based on domain (IPv4 or IPv6)
    if (domain == AF_INET) {

        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
        addr = (struct sockaddr *) &addr_in;

    } else if (domain == AF_INET6) {

        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_port = htons(port);
        addr_in6.sin6_addr = in6addr_any;
        addr = (struct sockaddr *) &addr_in6;
    } else {
        fprintf(stderr, "Unsupported domain.\n");
        return -1;
    }

// Bind socket to address
    if (bind(socket_fd, addr, (domain == AF_INET) ? sizeof(addr_in) : sizeof(addr_in6)) == -1) {
        perror("bind");
        return -1;
    }

// Start listening for incoming connections
    if (listen(socket_fd, SOMAXCONN) == -1) {
        perror("listen");
        return -1;
    }

    int client_sock;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    if ((client_sock = accept(socket_fd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    close(socket_fd);
    socket_fd = client_sock;
    return socket_fd;
}

int create_udp_server_socket(char *ip, long port, int domain) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr *addr;
    int yes = 1;
    char port_str[6]; // 5 digits for port number + null terminator
    sprintf(port_str, "%ld", port);


// Create socket
    int socket_fd = socket(domain, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Allow the socket to be reused immediately after program exit
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

// Set up the server address structure based on domain (IPv4 or IPv6)
    if (domain == AF_INET) {

        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
        addr = (struct sockaddr *) &addr_in;
    } else if (domain == AF_INET6) {

        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_port = htons(port);
        addr_in6.sin6_addr = in6addr_any;
        addr = (struct sockaddr *) &addr_in6;
    } else {
        fprintf(stderr, "Unsupported domain.\n");
        return -1;
    }


    // Bind socket to address
    if (bind(socket_fd, addr, (domain == AF_INET) ? sizeof(addr_in) : sizeof(addr_in6)) == -1) {
        perror("bind");
        return -1;
    }

    return socket_fd;
}

int create_uds_server_socket(char *filename, int is_stream) {
    struct sockaddr_un addr;
    int yes = 1;

// Create socket
    int socket_fd = socket(AF_UNIX, is_stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Allow the socket to be reused immediately after program exit
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return -1;
    }

// Remove existing file
    if (unlink(filename) == -1 && errno != ENOENT) {
        perror("unlink");
        return -1;
    }

// Set up the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);

// Bind socket to address
    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        return -1;
    }

// Start listening if stream socket
    if (is_stream && listen(socket_fd, 10) == -1) {
        perror("listen");
        return -1;
    }

    return socket_fd;
}

int create_tcp_client_socket(char *ip, long port, int domain) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr *addr;
    char port_str[6]; // 5 digits for port number + null terminator
    sprintf(port_str, "%ld", port);

// Create socket
    int socket_fd = socket(domain, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Set up the server address structure based on domain (IPv4 or IPv6)
    if (domain == AF_INET) {
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &(addr_in.sin_addr)) <= 0) {
            perror("inet_pton");
            return -1;
        }
        addr = (struct sockaddr *) &addr_in;
    } else if (domain == AF_INET6) {
        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip, &(addr_in6.sin6_addr)) <= 0) {
            perror("inet_pton");
            return -1;
        }
        addr = (struct sockaddr *) &addr_in6;
    } else {
        fprintf(stderr, "Unsupported domain.\n");
        return -1;
    }

// Connect socket to server
    if (connect(socket_fd, addr, (domain == AF_INET) ? sizeof(addr_in) : sizeof(addr_in6)) == -1) {
        perror("connect");
        return -1;
    }

    return socket_fd;
}

int create_udp_client_socket(char *ip, long port, int domain) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
    struct sockaddr *addr;
    char port_str[6]; // 5 digits for port number + null terminator
    sprintf(port_str, "%ld", port);

// Create socket
    int socket_fd = socket(domain, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Set up the server address structure based on domain (IPv4 or IPv6)
    if (domain == AF_INET) {
        addr_in.sin_family = AF_INET;
        addr_in.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &(addr_in.sin_addr)) <= 0) {
            perror("inet_pton");
            return -1;
        }
        addr = (struct sockaddr *) &addr_in;
    } else if (domain == AF_INET6) {
        addr_in6.sin6_family = AF_INET6;
        addr_in6.sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip, &(addr_in6.sin6_addr)) <= 0) {
            perror("inet_pton");
            return -1;
        }
        addr = (struct sockaddr *) &addr_in6;
    } else {
        fprintf(stderr, "Unsupported domain.\n");
        return -1;
    }

// Connect to the server
    if (connect(socket_fd, addr, (domain == AF_INET) ? sizeof(addr_in) : sizeof(addr_in6)) == -1) {
        perror("connect");
        return -1;
    }

    return socket_fd;
}

int create_uds_client_socket(char *filename, int is_stream) {
    struct sockaddr_un addr;

// Create socket
    int socket_fd = socket(AF_UNIX, is_stream ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        return -1;
    }

// Set up the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);

// Connect socket to server
    if (connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect");
        return -1;
    }

    return socket_fd;
}

int create_mmap_file(Connection *connection) {
// Check if the file already exists. If so, remove it.
    if (access(connection->filename, F_OK) != -1) {
        if (remove(connection->filename) == -1) {
            perror("remove");
            return -1;
        }
    }

// Open the file
    connection->mmap_fd = open(connection->filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (connection->mmap_fd == -1) {
        perror("open");
        return -1;
    }

// Set the size of the file to the size of the chunk
    if (ftruncate(connection->mmap_fd, CHUNK_SIZE) == -1) {
        perror("ftruncate");

// Cleanup and close the file descriptor in case of failure
        close(connection->mmap_fd);
        return -1;
    }
    connection->mmap_size = CHUNK_SIZE;

// Create the memory mapping
    connection->mmap_addr = mmap(NULL, connection->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, connection->mmap_fd,
                                 0);
    if (connection->mmap_addr == MAP_FAILED) {
        perror("mmap");

// Cleanup and close the file descriptor in case of failure
        close(connection->mmap_fd);
        return -1;
    }

    return 0;
}

int create_mmap_filer(Connection *connection) {
// Check if the file already exists. If so, remove it.

// Open the file
    connection->mmap_fd = open(connection->filename, O_RDWR, S_IRUSR | S_IWUSR);
    if (connection->mmap_fd == -1) {
        perror("openr");
        return -1;
    }

    connection->mmap_size = CHUNK_SIZE;

// Create the memory mapping
    connection->mmap_addr = mmap(NULL, connection->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, connection->mmap_fd,
                                 0);
    if (connection->mmap_addr == MAP_FAILED) {
        perror("mmap");

// Cleanup and close the file descriptor in case of failure
        close(connection->mmap_fd);
        return -1;
    }

    return 0;
}

int create_named_pipe(Connection *connection) {
    printf(" %s\n", connection->filename);

    mkfifo(connection->filename, 0666);
    // Open the pipe
    connection->pipe_fd[0] = open(connection->filename, O_RDONLY);
    printf("r %s\n", connection->filename);
    if (connection->pipe_fd[0] == -1) {
        perror("open");

        // Cleanup and close the file descriptors in case of failure
        if (connection->pipe_fd[0] != -1) close(connection->pipe_fd[0]);

        return -1;
    }
    return 0;
}

int create_named_pipew(Connection *connection) {

    // Create the named pipe (FIFO)
    mkfifo(connection->filename, 0666);

    // Open the pipe
    connection->pipe_fd[1] = open(connection->filename, O_WRONLY /*| O_NONBLOCK*/);
    printf("w %s\n", connection->filename);
    if (connection->pipe_fd[1] == -1) {
        perror("open");

        // Cleanup and close the file descriptors in case of failure
        if (connection->pipe_fd[1] != -1) close(connection->pipe_fd[1]);
        return -1;
    }
    return 0;
}

void destroy_connection(Connection *connection) {
    if (!connection) return; // Null check

    switch (connection->type) {
        case IPV4_TCP:
        case IPV4_UDP:
        case IPV6_TCP:
        case IPV6_UDP:
        case UDS_DGRAM:
        case UDS_STREAM:

            if (connection->socket_fd >= 0) {
                if (close(connection->socket_fd) == -1) {
                    perror("close");
                }
            }
            break;

        case MMAP:
            if (connection->mmap_addr) {
                if (munmap(connection->mmap_addr, connection->mmap_size) == -1) {
                    perror("munmap");
                }
            }
            if (connection->mmap_fd >= 0) {
                if (close(connection->mmap_fd) == -1) {
                    perror("close");
                }
            }
            break;

        case PIPE:
            break;
    }
    free(connection);
}

long send_data(Connection *connection, char *message, int len) {
    long buffer_size = MAX_BUFFER;
    long bytes_sent = 0;
    long nsent = 0;

    printf("connection->type %d", connection->type);
    switch (connection->type) {
        case IPV4_TCP:
        case IPV4_UDP:
        case IPV6_TCP:
        case IPV6_UDP:
        case UDS_DGRAM:
        case UDS_STREAM:


            while (nsent < len) {
                bytes_sent = send(connection->socket_fd, message + nsent, buffer_size, 0);
                if (bytes_sent == -1) {
                    perror("send");
                }
                nsent += bytes_sent;
            }
            printf("sent data of %ld byte\n", nsent);//.
            break;

        case MMAP:
            if (strlen(message) > connection->mmap_size) {
                fprintf(stderr, "Message too large for mmap size\n");
                bytes_sent = -1;
            } else {
                while (nsent < len) {
                    memcpy(connection->mmap_addr, message + nsent, buffer_size);
                    bytes_sent = buffer_size;
                    nsent += bytes_sent;
                }
            }
            // Write it now to disk
            if (msync(connection->mmap_addr, nsent, MS_SYNC) == -1) {
                perror("Could not sync the file to disk");
            }
            break;

        case PIPE:
            printf("before sending\n");
            while (nsent < len) {
                bytes_sent = write(connection->pipe_fd[1], message + nsent, buffer_size + 1);
                if (bytes_sent == -1) {
                    perror("write");
                }
                bytes_sent = buffer_size;
                nsent += bytes_sent;
            }
            close(connection->pipe_fd[1]);
            break;
    }

    return bytes_sent;
}

void handle_client_pB(Connection *conn) {
    char buffer[MAX_BUFFER];
    char *message;
    ssize_t bytes_received, total = 0;

    switch (conn->type) {
        case IPV4_TCP:
        case IPV4_UDP:
        case IPV6_TCP:
        case IPV6_UDP:
        case UDS_DGRAM:
        case UDS_STREAM:
            while ((bytes_received = recv(conn->socket_fd, buffer, MAX_BUFFER - 1, 0)) > 0) {
                buffer[bytes_received] = '\0';
                total += bytes_received;

                if (total >= CHUNK_SIZE)
                    break;
            }
            if (bytes_received == -1) {
                perror("recv");
                destroy_connection(conn);
                exit(EXIT_FAILURE);
            }
            break;

        case MMAP:
            // Copy received message from shared memory
            message = strdup(conn->mmap_addr);
            if (!message) {
                perror("strdup");
                destroy_connection(conn);
                exit(EXIT_FAILURE);
            }

            printf("Received: %s  %lu\n", message, strlen(message));

            // Echo message back to client
            free(message);

            break;

        case PIPE:

            while (1) {
                read(conn->pipe_fd[0], buffer, MAX_BUFFER);
                buffer[MAX_BUFFER - 1] = '\0';
            }
            close(conn->pipe_fd[0]);
            if (bytes_received == -1) {
                perror("read");
                destroy_connection(conn);
                exit(EXIT_FAILURE);
            }
            break;
    }

    printf("Client disconnected\n");
}

int main(int argc, char *argv[]) {
    // Parse the command line arguments
    int is_server = 0;
    int is_quiet = 0;
    char *ip = NULL;
    long port;
    char *type = NULL;
    char *param = NULL;

//------------------------------------P A R T   A------------------------------------------------------
    if ((strcmp(argv[1], "-c") == 0 && argc == 4) || (strcmp(argv[1], "-s") == 0 && argc == 3)) {
        int sockfd;
        struct sockaddr_in addr;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;

        if (strcmp(argv[1], "-c") == 0 && argc == 4) {
            addr.sin_port = htons(atoi(argv[3]));
            addr.sin_addr.s_addr = inet_addr(argv[2]);

            if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                perror("connect");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[1], "-s") == 0 && argc == 3) {
            addr.sin_port = htons(atoi(argv[2]));
            addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
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

            if ((client_sock = accept(sockfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            close(sockfd);
            sockfd = client_sock;
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }

        handle_client_pA(sockfd);
        close(sockfd);
    } else {

//------------------------------------P A R T   B------------------------------------------------------

//<type> ipv4,ipv6,mmap,pipe,uds
//<param> udp, tcp, dgram, stream, file name:

        Connection *conn = NULL;
        if (strcmp(argv[1], "-c") == 0) {
            if (argc != 7) {
                fprintf(stderr, "Error Usage:[-c <IP> <port> -p <type> <param>]\n");
                exit(EXIT_FAILURE);
            }
            ip = argv[2];
            port = atoi(argv[3]);
            if (strcmp(argv[4], "-p") != 0) {
                fprintf(stderr, "Error Usage:[-c <IP> <port> -p <type> <param>]\n");
                exit(EXIT_FAILURE);
            }
            type = argv[5];
            param = argv[6];

            printf("First: ip: %s, port: %ld, type: %s, param: %s\n", ip, port, type, param);
            // Open the connection
            conn = open_connection(ip, port, type, param, is_server);

        } else if (strcmp(argv[1], "-s") == 0) {
            if (argc < 4 || argc > 5) {
                fprintf(stderr, "Error Usage:[-s <port> -p -q]\n");
                exit(EXIT_FAILURE);
            }
            is_server = 1;
            port = atoi(argv[2]);
            if (strcmp(argv[3], "-p") != 0) {
                fprintf(stderr, "Error Usage:[-s <port> -p -q]\n");
                exit(EXIT_FAILURE);

            }
            if (argc == 5) {
                if (strcmp(argv[4], "-q") != 0) {
                    fprintf(stderr, "Error Usage:[-s <port> -p -q]\n");
                    exit(EXIT_FAILURE);
                }
                is_quiet = 1;
            }
            //ServerInfo *serverInfo = getServerInfo(port);
            //            split_type_and_param(serverInfo->type, &type, &param);
            //param = serverInfo->param;
            //type = serverInfo->type;
            //printf("Param: %s, Type: %s\n", param, type);
            //ip = getIP();
            type = "ipv6";
            param = "udp";
            ip = "::1";

            printf("MAIN: Param: %s, Type: %s\n", param, type);
            printf("IP: %s, Port: %ld, Type: %s, Param: %s, server: %d\n", ip, port, type, param, is_server);

            conn = open_connection(ip, port, type, param, is_server);
            handle_client_pB(conn);
        } else {
            fprintf(stderr, "Error Usage: [-c <IP> <port> -p <type> <param>] OR [-s <port> -p -q]\n");
            exit(EXIT_FAILURE);
        }

        PerformanceTest *Performance = create_performance_test();
        // Transmit the data and measure the time
        transmit_data(conn, Performance);
        // Report the result
        if (!is_quiet) {
            report_result(Performance);
        }
        // Clean up
        destroy_connection(conn);
        return 0;
    }

}

