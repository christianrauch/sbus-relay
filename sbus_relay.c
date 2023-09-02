#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdbool.h>


// https://beej.us/guide/bgnet/

#define PROJECT_NAME "sbus-relay"

#define PORT "51324"

int main(int argc, char **argv) {
    if(argc != 1) {
        printf("%s takes no arguments.\n", argv[0]);
        return 1;
    }
    printf("This is project %s.\n", PROJECT_NAME);


    // OLD way:
//    const int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (socketfd == -1) {
//        perror("Failed to create socket: ");
//        return EXIT_FAILURE;
//    }

//    const struct sockaddr_in addr = {
//        .sin_family = AF_INET,
//        .sin_addr.s_addr = INADDR_ANY,
//        .sin_port = htons(51324),
//    };

//    if(bind(socketfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
//        perror("Failed to bind address: ");
//        return EXIT_FAILURE;
//    }

    // NEW way:
    const struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = NULL,
        .ai_canonname = NULL,
        .ai_next = NULL,
    };

    struct addrinfo *res;

    const int err = getaddrinfo(NULL, PORT, &hints, &res);
    if(err != 0) {
        fprintf(stderr, "Failed to get : %s\n", gai_strerror(err));
        return EXIT_FAILURE;
    }

    const int socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketfd == -1) {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Failed to bind address");
        return EXIT_FAILURE;
    }

    freeaddrinfo(res);

    while (true) {
        //
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len;
        ssize_t nread;
        #define BUF_SIZE 500
        char buf[BUF_SIZE];

        printf("recvfrom...\n"); fflush(stdout);
//        nread = recvfrom(socketfd, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
//        printf("nread: %zd\n", nread); fflush(stdout);
//        if (nread == -1)
//            continue;

//        printf("buf: %s\n", buf); fflush(stdout);

        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(socketfd, buf, BUF_SIZE, 0,
                         (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1)
            continue;

        char host[NI_MAXHOST], service[NI_MAXSERV];

        int s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
        if (s == 0)
            printf("Received %ld bytes from %s:%s\n",
                   (long) nread, host, service);
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    }

    close(socketfd);


    return EXIT_SUCCESS;
}
