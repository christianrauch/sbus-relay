#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
//#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
//#include <netinet/in.h>
#include <stdbool.h>
#include <pty.h>
#include <signal.h>
#include <fcntl.h>


// https://beej.us/guide/bgnet/

#define PROJECT_NAME "sbus-relay"

#define PORT 51324

#define LINK "/tmp/SBUS"

int bind_socket(in_port_t port) {
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

    // convert port from int to string
//    int length = snprintf_s(NULL, 0, "%d", port);
    int length = snprintf(NULL, 0, "%d", port);
    char* str = malloc( length + 1 );
    snprintf(str, length + 1, "%d", port);

    struct addrinfo *res;

    const int err = getaddrinfo(NULL, str, &hints, &res);
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

    free(str);

    freeaddrinfo(res);

    return socketfd;
}

bool running = true;

void sig_handler(int signum){
    printf("sign %d\n", signum);
//    signal(SIGINT,SIG_DFL);   // Re Register signal handler for default action
    if (signum == SIGINT) {
        running = false;
    }
//    if (unlink(LINK) == -1) {
//        perror("Failed to unlink");
////        return EXIT_FAILURE;
//    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if(argc != 1) {
        printf("%s takes no arguments.\n", argv[0]);
        return 1;
    }
    printf("This is project %s.\n", PROJECT_NAME);

//    int master;
//    int slave;
//    char name[200];
//    if (openpty(&master, &slave, name, NULL, NULL) == -1) {
//        perror("Failed to create PTY");
//        return EXIT_FAILURE;
//    }

////    fd_set rfds;
////    FD_ZERO(&rfds);
////    FD_SET(master, &rfds);

//    /* serial port parameters */
//    //#define BAUDRATE    B100000
//    #define BAUDRATE B115200
//    struct termios newtio;
//    memset(&newtio, 0, sizeof(newtio));
//    struct termios oldtio;
//    tcgetattr(master, &oldtio);

//    newtio = oldtio;
//    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
//    newtio.c_iflag = 0;
//    newtio.c_oflag = 0;
//    newtio.c_lflag = 0;
//    newtio.c_cc[VMIN] = 1;
//    newtio.c_cc[VTIME] = 0;
//    tcflush(master, TCIFLUSH);

//    printf("pty %s.\n", name);

//    if (access(LINK, F_OK) == 0) {
//        // symlink already exist
//        if (unlink(LINK) == -1) {
//            perror("Failed to unlink");
//            return EXIT_FAILURE;
//        }
//    }

//    if (symlink(name, LINK) == -1) {
//        perror("Failed to create symlink");
////        if (errno == EEXIST) {
////            unlink("/tmp/SBUS");
////        }
//        return EXIT_FAILURE;
//    }

    // via tty0tty

    int master = open("/dev/tnt1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (master == -1) { perror("open_port: Unable to open port − "); }
    else {fcntl(master, F_SETFL, 0);}


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

//    int n = write(fd, "ATZ\r", 4);
//    if (n < 0)
//        fputs("write() of 4 bytes failed!\n", stderr);

    // NEW way:
    const int socketfd = bind_socket(PORT);

    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);

    printf("sbus\n"); fflush(stdout);
    // https://github.com/bolderflight/sbus/blob/main/README.md
//    struct channel_t { int value : 11; };
//    struct sbus_t {
////        uint8_t header;
//        struct channel_t channel [16];
////        uint8_t status;
////        uint8_t footer;
//    };
//    printf("sbus_t size: %lu\n", sizeof(struct sbus_t)); fflush(stdout);
    uint8_t sbus[25];
    memset(sbus, 0, sizeof(sbus));
    sbus[0] = 0x0F;
//    sbus[1] = 100;
//    struct sbus_t sbus;
//    sbus.header = 0x0F;
//    sbus.channel[0].value = 100;
//    while (running) {
////        write(master, sbus, sizeof (sbus));
//        write(fd, sbus, sizeof (sbus));
//    }

//    return 0;

    while (running) {
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
        if (s == 0) {
            printf("Received %ld bytes from %s:%s\n",
                   (long) nread, host, service);
            printf("buf: %s\n", buf); fflush(stdout);
        }
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

        // reset
        memset(sbus, 0, sizeof(sbus));
        sbus[0] = 0x0F;
//        ssize_t size = read(master, buf, BUF_SIZE);
//        write(master, buf, nread);
        memcpy(&sbus[1], buf, 23);
//        write(fd, buf, nread);
//        write(fd, sbus, 24);
//        sbus[1] = buf[0];
        write(master, sbus, sizeof (sbus));
    }

    printf("bye!\n"); fflush(stdout);

    close(socketfd);



    return EXIT_SUCCESS;
}
