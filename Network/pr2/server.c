// Author: Ruiqi Wang

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <errno.h>          /* for errno */

#define _GNU_SOURCE
#define STRING_SIZE 256
#define LINE_SIZE 80
#define RELIABLE    // if define RELIABLE, program will run a reliable data transmission

// #define DEBUG   // if define DEBUG, program will test the case with missing packet

#ifdef DEBUG
#include <time.h>
#endif

// data packet structure
typedef struct _data_pkt{
    unsigned short msg_len;
    unsigned short seq_num;
    char context[STRING_SIZE];
} _data_pkt;

// ack packet structure
typedef struct _ack_pkt{
    unsigned short seq_num;
} _ack_pkt;

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

int main(int args, char **argv) {

    int total_pkt = 0, total_ack = 0, total_timeo = 0;
    long total_byte = 0;

    /* Socket on which server listens to clients */

    int sock_server;

    /* Internet address structure that stores server address */

    struct sockaddr_in server_addr;

    /* Internet address structure that stores client address */

    struct sockaddr_in client_addr;

    /* Length of client address structure */

    unsigned int client_addr_len;

    char context[STRING_SIZE];
    unsigned long context_len = 0;
    unsigned short recv_seq, seq_num = 0;

    struct timeval timeout;

    _data_pkt packet;
    _ack_pkt ack;

    FILE *outpFile = NULL;

    int finish = 0;

    /* open a socket */

    if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Server: can't open datagram socket");
        exit(1);
    }

    /* initialize server address information */

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    server_addr.sin_port = htons(args>1?atoi(argv[1]):12000);

    /* bind the socket to the local server port */

    if (bind(sock_server, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
        perror("Server: can't bind to local address");
        close(sock_server);
        exit(1);
    }

    client_addr_len = sizeof (client_addr);

    #ifdef DEBUG
    srand(time(NULL));
    #endif

    /* receive file name */
    recvfrom(sock_server, &packet, sizeof(_data_pkt), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    recv_seq = ntohs(packet.seq_num);

    outpFile = fopen(packet.context, "r");

    // setup timer, timeout 1 sec
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
        perror("Server: cannot setup timer\n");
        fclose(outpFile);
        close(sock_server);
        exit(1);
    }

    while(!finish){
        if(NULL == fgets(context, STRING_SIZE, outpFile)){
            finish = 1;
            packet.msg_len = htons(0);
            packet.seq_num = htons(seq_num);
            packet.context[0] = 0;
            sendto(sock_server, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, client_addr_len);
            printf("End of Transmission Packet with sequence number %hu transmitted\n", seq_num);
        }
        else{
            context_len = strlen(context);
            context_len = context_len > LINE_SIZE ? LINE_SIZE : context_len;
            packet.msg_len = htons(context_len);
            packet.seq_num = htons(seq_num);
            memcpy(packet.context, context, context_len);
            packet.context[context_len] = 0;
        }

        // if reliable, setup resend flag
        #ifdef RELIABLE
    SEND_CONTEXT:
        #endif

        // random missing packets
        #ifdef DEBUG
        if(1.*rand()/RAND_MAX <= .2){
            printf("-- LOST --\n");
            goto PKT_LOST;
        }
        #endif

        // send data packet
        sendto(sock_server, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, client_addr_len);

        #ifdef DEBUG
    PKT_LOST:
        #endif

        if(!finish){
            // count total bytes and total packets
            total_byte += context_len+4;
            total_pkt++;
            printf("Packet %hu transmitted with %lu data bytes\n", seq_num, context_len+4);
        }

        // if reliable, setup waiting loop
        #ifdef RELIABLE
    WAIT_FOR_ACK:
        #endif

        // receive ack
        recvfrom(sock_server, &ack, sizeof(_ack_pkt), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        recv_seq = ntohs(ack.seq_num);
        if(errno == EAGAIN){
            // timeout, reset error number and timeout counter add 1
            errno = 0;
            total_timeo++;
            printf("Timeout expired for packet numbered %hu\n", seq_num);
            // if reliable, resend packet
        #ifdef RELIABLE
            goto SEND_CONTEXT;
        #endif
        }
        // if(recv_seq == seq_num)
        printf("ACK %hu received\n", recv_seq);

        // if reliable, do nothing when the packet corrupt but wait for timeout
        #ifdef RELIABLE
        if(recv_seq != seq_num)
            goto WAIT_FOR_ACK;
        #endif

        if(!finish)
            total_ack++;
        seq_num++;
        seq_num %= 16;
        putchar('\n');
    }

    printf("====================\n");
    printf("total packets: %d\n", total_pkt);
    printf("total data bytes: %ld\n", total_byte);
    printf("total ack: %d\n", total_ack);
    printf("total timeout: %d\n", total_timeo);
    printf("====================\n");

    /* close the file and socket */

    fclose(outpFile);
    close(sock_server);
    return 0;
}
