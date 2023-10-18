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
//#include <pty.h>
#include <signal.h>
#include <fcntl.h>
#include  <math.h>
//#include <termios.h>
//#include <sys/ioctl.h>
#include <asm/termios.h>

#define _GNU_SOURCE
#include <stdio.h>


#define PORT 51324

//#define MASTER "/dev/vtmx"

#define MASTER "/dev/tnt0"

//#define LINK_SLAVE "/tmp/SBUS"

#define MAX_CHANNELS 16
// uint8 magic + uint8 len + 16 * float32 (4) = 66 bytes
#define BUF_MAX_SIZE sizeof(uint8_t) + sizeof(uint8_t) + sizeof(float) * MAX_CHANNELS


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
    char **port_str = NULL;
    if (asprintf(port_str, "%d", port) < 0) {
        fprintf(stderr, "Failed to convert port\n");
        return -1;
    }

    struct addrinfo *res;

    const int err = getaddrinfo(NULL, *port_str, &hints, &res);
    if(err != 0) {
        fprintf(stderr, "Failed to get : %s\n", gai_strerror(err));
        return -1;
    }

    const int socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketfd == -1) {
        perror("Failed to create socket");
        return -1;
    }

    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Failed to bind address");
        return -1;
    }

    free(port_str);

    freeaddrinfo(res);

    return socketfd;
}

bool deserialise_channels(const uint8_t (*const buf)[BUF_MAX_SIZE], float (*const channels)[MAX_CHANNELS])
{
    if ((*buf[0]) != 0x0F)
    {
        fprintf(stderr, "invalid package!\n");
        return false;
    }

    const uint8_t n = (*buf)[1]; // number of sent channels

    if (n > MAX_CHANNELS)
    {
        fprintf(stderr, "Too many channels (%d)!\n", n);
        return false;
    }

    for(size_t i=0; i<n; i++)
    {
        const uint32_t bb = ((*buf)[2 + i*4] << 24) | ((*buf)[2 + i*4 + 1] << 16) | ((*buf)[2 + i*4 + 2] << 8) | ((*buf)[2 + i*4 + 3]);
        (*channels)[i] = *(float *const)&bb;
    }

    return true;
}

void serialise_channels(const float (*const channels)[MAX_CHANNELS], uint8_t (*const sbus)[25])
{
    // https://github.com/bolderflight/sbus/blob/main/README.md
    memset(sbus, 0, 25 * sizeof(uint8_t));
    (*sbus)[0] = 0x0F; // header
    (*sbus)[24] = 0x00; // footer

    uint16_t channels_us[MAX_CHANNELS];

    // map 0...1 to 0...2000 us
    for(size_t i=0; i<MAX_CHANNELS; i++)
    {
        channels_us[i] = 2000 * fmaxf(0, fminf((*channels)[i], 1));
    }

    // pack channels as 11bit integer

    // |00.....|01.....|02.....|03.....|04.....|05.....|06.....|07.....|08.....|09.....|10..... (11 * 8bits)
    // |00........|01........|02........|03........|04........|05........|06........|07........ (8 * 11bits)

    // |11.....|12.....|13.....|14.....|15.....|16.....|17.....|18.....|19.....|20.....|21..... (11 * 8bits)
    // |08........|09........|10........|11........|12........|13........|14........|15........ (8 * 11bits)

    // https://github.com/Reefwing-Software/Reefwing-SBUS/blob/2.0.1/src/ReefwingSBUS.cpp#L188-L209
    (*sbus)[1] = (uint8_t) ((channels_us[0] & 0x07FF));
    (*sbus)[2] = (uint8_t) ((channels_us[0] & 0x07FF)>>8 | (channels_us[1] & 0x07FF)<<3);
    (*sbus)[3] = (uint8_t) ((channels_us[1] & 0x07FF)>>5 | (channels_us[2] & 0x07FF)<<6);
    (*sbus)[4] = (uint8_t) ((channels_us[2] & 0x07FF)>>2);
    (*sbus)[5] = (uint8_t) ((channels_us[2] & 0x07FF)>>10 | (channels_us[3] & 0x07FF)<<1);
    (*sbus)[6] = (uint8_t) ((channels_us[3] & 0x07FF)>>7 | (channels_us[4] & 0x07FF)<<4);
    (*sbus)[7] = (uint8_t) ((channels_us[4] & 0x07FF)>>4 | (channels_us[5] & 0x07FF)<<7);
    (*sbus)[8] = (uint8_t) ((channels_us[5] & 0x07FF)>>1);
    (*sbus)[9] = (uint8_t) ((channels_us[5] & 0x07FF)>>9 | (channels_us[6] & 0x07FF)<<2);
    (*sbus)[10] = (uint8_t) ((channels_us[6] & 0x07FF)>>6 | (channels_us[7] & 0x07FF)<<5);
    (*sbus)[11] = (uint8_t) ((channels_us[7] & 0x07FF)>>3);
    (*sbus)[12] = (uint8_t) ((channels_us[8] & 0x07FF));
    (*sbus)[13] = (uint8_t) ((channels_us[8] & 0x07FF)>>8 | (channels_us[9] & 0x07FF)<<3);
    (*sbus)[14] = (uint8_t) ((channels_us[9] & 0x07FF)>>5 | (channels_us[10] & 0x07FF)<<6);
    (*sbus)[15] = (uint8_t) ((channels_us[10] & 0x07FF)>>2);
    (*sbus)[16] = (uint8_t) ((channels_us[10] & 0x07FF)>>10 | (channels_us[11] & 0x07FF)<<1);
    (*sbus)[17] = (uint8_t) ((channels_us[11] & 0x07FF)>>7 | (channels_us[12] & 0x07FF)<<4);
    (*sbus)[18] = (uint8_t) ((channels_us[12] & 0x07FF)>>4 | (channels_us[13] & 0x07FF)<<7);
    (*sbus)[19] = (uint8_t) ((channels_us[13] & 0x07FF)>>1);
    (*sbus)[20] = (uint8_t) ((channels_us[13] & 0x07FF)>>9 | (channels_us[14] & 0x07FF)<<2);
    (*sbus)[21] = (uint8_t) ((channels_us[14] & 0x07FF)>>6 | (channels_us[15] & 0x07FF)<<5);
    (*sbus)[22] = (uint8_t) ((channels_us[15] & 0x07FF)>>3);
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

    // via vtty

//    #define SLAVE_PATH "/dev/ttyV0"

    int master = open(MASTER, O_WRONLY);
    if (master == -1) { perror("Unable to open master port"); }
//    else {fcntl(master, F_SETFL, 0);}

//#define VTMX_GET_VTTY_NUM (TIOCGPTN)
//#define VTMX_SET_MODEM_LINES (TIOCMSET)
//    unsigned int rv = ~0;
//    ioctl(master, VTMX_GET_VTTY_NUM, &rv);
//    char slave_path[100];
//    snprintf(slave_path, 100, "/dev/ttyV%d", rv);
//    printf("slave: %s\n", slave_path);

//    unsigned int a;
//    a = TIOCM_RTS /* should be ignored */ | TIOCM_CTS;
//    ioctl(master, VTMX_SET_MODEM_LINES, &a);

//    #define SBUS_BAUD 100000
//    struct termios2 tio;
//    ioctl(master, TCGETS2, &tio);
//    tio.c_cflag &= ~CBAUD;
//    tio.c_cflag |= BOTHER;
//    tio.c_ospeed = SBUS_BAUD;
//    tio.c_cflag &= ~(CBAUD << IBSHIFT);
//    tio.c_cflag |= BOTHER << IBSHIFT;
//    tio.c_ispeed = SBUS_BAUD;
//    ioctl(master, TCSETS2, &tio);

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
//    uint8_t sbus[25] = {0};
//    sbus[0] = 0x0F;
//    sbus[1] = 100;
//    struct sbus_t sbus;
//    sbus.header = 0x0F;
//    sbus.channel[0].value = 100;
//    while (running) {
////        write(master, sbus, sizeof (sbus));
//        write(fd, sbus, sizeof (sbus));
//    }

//    return 0;

    // example how to format and send channel data:
    //  import socket, struct; socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(bytearray(struct.pack("!BBffff", 0x0F, 4, *(0.5, 0.25, 0.3, 1.0))), ("127.0.0.1", 51324))

#if 1
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
#endif
    ssize_t nread;
    uint8_t buf[BUF_MAX_SIZE];

    float channels[MAX_CHANNELS];
    uint8_t sbus[25];

    while (running) {
        //

//        buf = {0};
        memset(buf, 0, sizeof(buf));

        printf("recvfrom...\n"); fflush(stdout);
//        nread = recvfrom(socketfd, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
//        printf("nread: %zd\n", nread); fflush(stdout);
//        if (nread == -1)
//            continue;

//        printf("buf: %s\n", buf); fflush(stdout);

        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(socketfd, buf, BUF_MAX_SIZE, 0,
#if 1
                         (struct sockaddr *) &peer_addr, &peer_addr_len
#endif
                         );
        if (nread == -1) {
            perror("Failed to read from socket");
            continue;
        }

        if ((uint8_t)nread > BUF_MAX_SIZE)
        {
            fprintf(stderr, "Message too large!\n");
            continue;
        }

//        if (buf[0] != 0x0F)
//        {
//            fprintf(stderr, "invalid package!\n");
//            continue;
//        }

        const uint8_t n = buf[1];
        if (n * sizeof(float) > (uint8_t)nread)
        {
            fprintf(stderr, "Too many channels (%d)!\n", n);
            continue;
        }

#if 1
        char host[NI_MAXHOST], service[NI_MAXSERV];

        int s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
        if (s == 0) {
            printf("Received %ld bytes from %s:%s\n", (long) nread, host, service);
//            printf("buf: %s\n", buf); fflush(stdout);
        }
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
#endif

        // convert data
//        float channels[MAX_CHANNELS] = {0};
//        uint8_t sbus[25] = {0};
        memset(channels, 0, sizeof(channels));
        memset(sbus, 0, sizeof(sbus));
        deserialise_channels(&buf, &channels);
        serialise_channels(&channels, &sbus);

//        printf("Received %ld bytes from %s:%s\n", (long) nread, host, service);
        printf("channels(%d): ", n);
        for(int i=0; i<MAX_CHANNELS; i++) {
            printf("%f, ", channels[i]);
        }
        printf("\n");

        write(master, sbus, 25 * sizeof(uint8_t));
    }

    printf("bye!\n"); fflush(stdout);

    close(socketfd);

    return EXIT_SUCCESS;
}
