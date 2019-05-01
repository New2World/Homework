// Author: Ruiqi Wang

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <arpa/inet.h>      /* for inet_addr */

#define STRING_SIZE 256
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

int main(int args, char **argv) {

    int total_pkt = 0, total_ack = 0;
    #ifdef DEBUG
    int total_timeo = 0;
    #endif
    long total_byte = 0;
    int sock_client;  /* Socket used by client */

    /* Internet address structure that stores client address */

    struct sockaddr_in client_addr;

    /* Internet address structure that stores server address */

    struct sockaddr_in server_addr;

    char fileName[STRING_SIZE];
    unsigned short msg_len;/* length of message */
    unsigned short recv_len, recv_seq;
    unsigned short seq_num = 0;

    _data_pkt packet;
    _ack_pkt ack;

    FILE *outpFile = fopen("out.txt", "w");

    /* open a socket */

    if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Client: can't open datagram socket");
        fclose(outpFile);
        exit(1);
    }

    /* clear client address structure and initialize with client address */

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(args>1?atoi(argv[1]):12001);

    /* bind the socket to the local client port */

    if (bind(sock_client, (struct sockaddr *) &client_addr, sizeof (client_addr)) < 0) {
        perror("Client: can't bind to local address");
        close(sock_client);
        fclose(outpFile);
        exit(1);
    }

    /* initialize server address information */

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(args>1?atoi(argv[1]):12000);

    /* user interface */

    printf("Choose the file: ");
    fflush(stdout);
    scanf("%s", fileName);
    msg_len = strlen(fileName);

    #ifdef DEBUG
    srand(time(NULL));
    #endif

    packet.msg_len = htons(msg_len);
    packet.seq_num = htons(seq_num);
    memcpy(packet.context, fileName, msg_len);
    packet.context[msg_len] = 0;

    /* send file name */
    sendto(sock_client, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    /* get response from server */

    do{
        // receive data packet from server and get packet length and sequence
        recvfrom(sock_client, &packet, sizeof(_data_pkt), 0, NULL, NULL);
        recv_seq = ntohs(packet.seq_num);
        recv_len = ntohs(packet.msg_len);

        // end of transmission
        if(recv_len == 0){
            printf("\nEnd of Transmission Packet with sequence number %hu received\n", recv_seq);
        }
        else{
            total_byte += recv_len+4;
            printf("\nPacket %hu received with %u data bytes\n", recv_seq, recv_len+4U);
        }

        // random timeout ack
        #ifdef DEBUG
        if(1.*rand()/RAND_MAX <= .2){
            total_timeo++;
            printf("-- TIMEOUT --\n");
            sleep(2);
        }
        #endif

        ack.seq_num = htons(recv_seq);

        // random missing ack
        #ifdef DEBUG
        if(1.*rand()/RAND_MAX <= .1){
            printf("-- LOST --\n");
            goto PKT_LOST;
        }
        #endif

        // send ack
        sendto(sock_client, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        #ifdef DEBUG
    PKT_LOST:
        #endif

        printf("ACK %hu transmitted\n", recv_seq);
        if(recv_len > 0){
            total_pkt++;
            total_ack++;
        }
        // if sequence number equals to received sequence, write the data to file
        if(seq_num == recv_seq){
            seq_num++;
            seq_num %= 16;
            fwrite(packet.context, sizeof(char), recv_len, outpFile);
        }
        // if not reliable, left the missing packet a empty line
        #ifndef RELIABLE
        else{
            seq_num++;
            seq_num %= 16;
            fwrite("\n", sizeof(char), 1, outpFile);
        }
        #endif
    }while(recv_len > 0);

    printf("\n====================\n");
    printf("total packets: %d\n", total_pkt);
    printf("total data bytes: %ld\n", total_byte);
    printf("total ack: %d\n", total_ack);

    #ifdef DEBUG
    printf("total timeout: %d\n", total_timeo);
    #endif

    printf("====================\n");

    /* close the file and socket */

    fclose(outpFile);
    close (sock_client);
}
