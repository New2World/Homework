// Author: Ruiqi Wang

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <arpa/inet.h>      /* for inet_addr */
#include <time.h>

#define STRING_SIZE 80
#define CLIENT_PORT 12001
#define SERVER_PORT 12000

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

// make ACK packet with sequence number
void make_ack(_ack_pkt &ack, unsigned short seq_num){
    ack.seq_num = htons(seq_num);
    printf("ACK %hu generated for transmission\n", seq_num);
}

// make data packet of one line from the file
void make_pkt(_data_pkt &pkt, char *msg, unsigned short seq_num){
    int msg_len = strlen(msg);
    pkt.msg_len = htons(msg_len);
    pkt.seq_num = htons(seq_num);
    memcpy(pkt.context, msg, msg_len);
    pkt.context[msg_len] = 0;
}

// get message length from packet
unsigned short get_msglen(_data_pkt pkt){
    return ntohs(pkt.msg_len);
}

// get sequence number from packet
unsigned short get_seqnum_pkt(_data_pkt pkt){
    return ntohs(pkt.seq_num);
}

// get sequence number from ack
unsigned short get_seqnum_ack(_ack_pkt ack){
    return ntohs(ack.seq_num);
}

// determine if packet is in sequence
bool has_seq(_data_pkt pkt, unsigned short seq_num){
    if(seq_num == get_seqnum_pkt(pkt)){
        printf("Packet %hu received with %hu data bytes\n", seq_num, get_msglen(pkt)+4);
        return true;
    }
    printf("Duplicate packet %hu received with %hu data bytes\n", seq_num, get_msglen(pkt)+4);
    return false;
}

// write the message in packet to output file
bool deliver_data(_data_pkt pkt){
    FILE *outpFile = fopen("out.txt", "a");
    unsigned short msg_len = get_msglen(pkt);
    unsigned short seq_num = get_seqnum_pkt(pkt);
    if(msg_len == 0){
        fclose(outpFile);
        printf("End of Transmission Packet with sequence number %hu transmitted\n", seq_num);
        return true;
    }else{
        fwrite(pkt.context, sizeof(char), msg_len, outpFile);
        printf("Packet %hu delivered to user\n", seq_num);
    }
    fclose(outpFile);
    return false;
}

int main(int args, char **argv) {
    int n_pkt_succ, n_pkt_fail, n_pkt;
    int n_byte_deliv;
    int n_ack_succ, n_ack_fail, n_ack;
    float loss_ratio = .1;

    int sock_client = 0;  /* Socket used by client */

    /* Internet address structure that stores client address */
    struct sockaddr_in client_addr;

    /* Internet address structure that stores server address */
    struct sockaddr_in server_addr;

    bool finish = false;
    char fileName[STRING_SIZE];
    unsigned short msg_len;/* length of message */
    unsigned short seq_num = 1;

    srand(time(NULL));

    /************************************/
    /* create socket and initialization */
    /************************************/

    /* open a socket */
    if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Client: can't open datagram socket");
        exit(1);
    }

    /* clear client address structure and initialize with client address */
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(CLIENT_PORT);

    /* bind the socket to the local client port */
    if (bind(sock_client, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Client: can't bind to local address");
        close(sock_client);
        exit(1);
    }

    /* initialize server address information */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    /* user interface */
    printf("Choose the file: ");
    fflush(stdout);
    scanf("%s", fileName);
    msg_len = strlen(fileName);
    printf("ACK loss ratio: ");
    fflush(stdout);
    scanf("%f", &loss_ratio);
    putchar('\n');

    _data_pkt pkt;
    _ack_pkt ack;

    make_pkt(pkt, fileName, seq_num);

    /* send file name */
    sendto(sock_client, &pkt, sizeof(pkt), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    FILE *outpFile = fopen("out.txt", "w");
    fclose(outpFile);

    n_pkt_succ = 0;
    n_pkt_fail = 0;
    n_pkt = 0;
    n_byte_deliv = 0;
    n_ack_succ = 0;
    n_ack_fail = 0;
    n_ack = 0;

    /* get response from server */
    while(true){
        // receive data packet from server
        recvfrom(sock_client, &pkt, sizeof(_data_pkt), 0, NULL, NULL);
        n_pkt++;

        if(get_msglen(pkt) <= 0)
            break;

        // expected sequence number received
        if(has_seq(pkt, seq_num)){
            n_pkt_succ++;
            finish = deliver_data(pkt);     // return if receive the empty packet
            make_ack(ack, seq_num);
            n_byte_deliv += get_msglen(pkt);
            seq_num++;
            seq_num %= 16;
        }else
            n_pkt_fail++;

        // drop ack randomly
        if(1.*rand()/RAND_MAX > loss_ratio){
            // send ack to server
            sendto(sock_client, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            printf("ACK %hu successfully transmitted\n", get_seqnum_ack(ack));
            n_ack_succ++;
        }else{
            printf("ACK %hu lost\n", get_seqnum_ack(ack));
            n_ack_fail++;
        }
        n_ack++;

        // if receive empty packet, exit the loop
        if(finish)
            break;
    }

    // output information
    printf("\n====================\n");
    printf("total packets received successfully: %d\n", n_pkt_succ);
    printf("total data bytes delivered: %d\n", n_byte_deliv);
    printf("total duplicate or out-of-senquence packets: %d\n", n_pkt_fail);
    printf("total packets received: %d\n", n_pkt);
    printf("total ACKs transmitted: %d\n", n_ack_succ);
    printf("total ACKs dropped: %d\n", n_ack_fail);
    printf("total ACKs: %d\n", n_ack);
    printf("====================\n");

    /* close the file and socket */
    close(sock_client);

    return 0;
}
