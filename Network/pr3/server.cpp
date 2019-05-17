// Author: Ruiqi Wang

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <errno.h>          /* for errno */
#include <math.h>
#include <time.h>
#include <queue>

using namespace std;

#define STRING_SIZE 80
#define QUE_LEN 10
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

// make data packet
void make_pkt(_data_pkt &pkt, char *msg, unsigned short seq_num){
    int msg_len = strlen(msg);
    pkt.msg_len = htons(msg_len);
    pkt.seq_num = htons(seq_num);
    memcpy(pkt.context, msg, msg_len);
    pkt.context[msg_len] = 0;
    printf("Packet %hu generated for transmission with %hu data bytes\n", seq_num, msg_len+4);
}

// extract sequence number from data packet
unsigned short get_seqnum_pkt(_data_pkt pkt){
    return ntohs(pkt.seq_num);
}

// extract sequence number from ack packet
unsigned short get_seqnum_ack(_ack_pkt ack){
    return ntohs(ack.seq_num);
}

// extract packet length from data packet
unsigned short get_msglen(_data_pkt pkt){
    return ntohs(pkt.msg_len);
}

int main(int args, char **argv) {
    int n_pkt_re, n_pkt_succ, n_pkt_fail, n_pkt;
    int n_byte_gen;
    int n_ack;
    int n_timeout;

    /* Socket on which server listens to clients */
    int sock_server;

    /* Internet address structure that stores server address */
    struct sockaddr_in server_addr;

    /* Internet address structure that stores client address */
    struct sockaddr_in client_addr;

    /* Length of client address structure */
    unsigned int client_addr_len;

    bool finish = false;
    char context[STRING_SIZE];
    unsigned short seq_num = 1, base_seq = 1, recv_seq;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    srand(time(NULL));

    _data_pkt pkt;
    _ack_pkt ack;
    queue<_data_pkt> pkt_que;

    FILE *inpFile = NULL;

    int window_size = 5, timeo = 3;
    float loss_ratio = .1;

    /* open a socket */
    if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("Server: can't open datagram socket");
        exit(1);
    }

    /* initialize server address information */
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    /* bind the socket to the local server port */
    if (bind(sock_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Server: can't bind to local address");
        close(sock_server);
        exit(1);
    }

    /* user interface */
    printf("Window size: ");
    fflush(stdout);
    scanf("%d", &window_size);
    printf("Timeout: ");
    fflush(stdout);
    scanf("%d", &timeo);
    printf("Packet loss ratio: ");
    fflush(stdout);
    scanf("%f", &loss_ratio);
    putchar('\n');

    client_addr_len = sizeof(struct sockaddr_in);

    /* receive file name */
    recvfrom(sock_server, &pkt, sizeof(_data_pkt), 0, (struct sockaddr *)&client_addr, &client_addr_len);

    inpFile = fopen(pkt.context, "r");

    // setup timer
    if(timeo == 6)
        timeout.tv_sec = 1;
    else if(timeo > 6){
        timeout.tv_sec = 1;
        for(int i = 0;i < timeo-6;i++)
            timeout.tv_sec *= 10;
    }else{
        timeout.tv_sec = 0;
        timeout.tv_usec = 1;
        for(int i = 0;i < timeo;i++)
            timeout.tv_usec *= 10;
    }
    if(setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
        perror("Server: cannot setup timer\n");
        fclose(inpFile);
        close(sock_server);
        exit(1);
    }

    // initialization
    n_pkt_re = 0;
    n_pkt_succ = 0;
    n_pkt_fail = 0;
    n_pkt = 0;
    n_byte_gen = 0;
    n_ack = 0;
    n_timeout = 0;

    while(true){
        while(!feof(inpFile) && pkt_que.size() < window_size){
            if(NULL == fgets(context, STRING_SIZE, inpFile))
                break;
            make_pkt(pkt, context, seq_num);
            n_pkt++;
            n_pkt_succ++;
            n_byte_gen += get_msglen(pkt);
            pkt_que.push(pkt);
            // drop packet with a random probability
            if(1.*rand()/RAND_MAX > loss_ratio){
                sendto(sock_server, &pkt, sizeof(pkt), 0, (struct sockaddr *)&client_addr, client_addr_len);
                printf("Packet %hu successfully transmitted with %hu data bytes\n", seq_num, get_msglen(pkt)+4);
            }else{
                printf("Packet %hu lost\n", seq_num);
                n_pkt_fail++;
            }
            seq_num++;
            seq_num %= 16;
        }

        if(pkt_que.empty()) break;

        // receive ack
        recvfrom(sock_server, &ack, sizeof(_ack_pkt), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        // timeout, reset error number and timeout counter add 1
        if(errno == EAGAIN){
            errno = 0;
            printf("Timeout expired for packet numbered %hu\n", base_seq);
            n_timeout++;
            for(int i = 0;i < pkt_que.size();i++){
                _data_pkt tmp_pkt = pkt_que.front();
                // re-transmit
                sendto(sock_server, &tmp_pkt, sizeof(tmp_pkt), 0, (struct sockaddr *)&client_addr, client_addr_len);
                printf("Packet %hu generated for re-transmission with %hu data bytes\n", get_seqnum_pkt(tmp_pkt), get_msglen(tmp_pkt)+4);
                n_pkt_re++;
                n_pkt++;
                pkt_que.pop();
                pkt_que.push(tmp_pkt);
            }
        }else{
            recv_seq = get_seqnum_ack(ack);
            printf("ACK %hu received\n", recv_seq);
            n_ack++;
            base_seq = recv_seq+1;
            base_seq %= 16;
            while(!pkt_que.empty() && base_seq != get_seqnum_pkt(pkt_que.front()))
                pkt_que.pop();
        }
    }
    fclose(inpFile);

    context[0] = 0;
    make_pkt(pkt, context, seq_num);
    sendto(sock_server, &pkt, sizeof(pkt), 0, (struct sockaddr *)&client_addr, client_addr_len);
    printf("End of Transmission packet with sequence number %hu transmitted\n", seq_num);

    // output information
    printf("\n====================\n");
    printf("total packets generated for the first time: %d\n", n_pkt_succ);
    printf("total data bytes generated for the first time: %d\n", n_byte_gen);
    printf("total packets re-transmitted: %d\n", n_pkt_re);
    printf("total packets dropped: %d\n", n_pkt_fail);
    printf("total packets transmitted: %d\n", n_pkt);
    printf("total ACKs: %d\n", n_ack);
    printf("total timeout expired: %d\n", n_timeout);
    printf("====================\n");

    /* close the socket */
    close(sock_server);

    return 0;
}
