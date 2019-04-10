/*
* CISC650 - Network - Project I
* author: Ruiqi Wang
* date: 3/13/2019
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>     // for socket, connect, send and recv
#include <netinet/in.h>     // for sockaddr_in
#include <arpa/inet.h>      // for inet_addr

#define BUF_SIZE 1000
#define RECV_SIZE 80

int buf_num;

int count_num(int recv_len){
    /*
    split number in a string buffer using delimiter ' '
    */
    return recv_len/2;
}

int checksum_C(char *buf, int checksum){
    /*
    calculate checksum of characters one by one
    */
    int i = 0, len = strlen(buf);
    for(;i < len;i++){
        checksum += buf[i];
        checksum %= 256;
    }
    return checksum;
}

int checksum_N(char *buf, int recv_len, int checksum){
    /*
    calculate checksum of numbers one by one
    */
    int i = 0;
    unsigned short num[50];
    memcpy(num, buf, recv_len);
    for(;i < recv_len/2;i++){
        checksum += num[i];
        checksum %= 65536;
    }
    return checksum;
}

void delay(){
    /*
    delay, wait for transmission finished
    */
    int delay_num = (rand()%900+100)*3000;
    while(delay_num--);
}

int main(int argc, char **argv){
    int client_sock;
    int num = 0;
    int receive_num = 0;
    int separate_num = 0;
    int receive_byte = 0, recv_byte;
    int checksum = 0;
    struct sockaddr_in server_addr;
    char option;
    char buf[BUF_SIZE] = {0};

    // initialize server address information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(argc>1?atoi(argv[1]):12000);
    CONNECT:
    // create socket
    if((client_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation");
        return 1;
    }
    // connect to server
    if(connect(client_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("connection");
        return 1;
    }

    // clear variables to be used
    buf[0] = 0;
    num = 0;
    receive_num = 0;
    separate_num = 0;
    receive_byte = 0;
    checksum = 0;
    buf_num = 0;

    while(buf[0] != 'N' && buf[0] != 'C'){
        printf("Choose C(haracter) or N(umber) [C/N]: ");
        fflush(stdout);
        scanf("%s", buf);
        option = buf[0];
    }
    while(num < 1 || num > 65535){
        printf("Enter the number of %s you want to get from server: ", buf[0]=='C'?"letters":"integers");
        fflush(stdout);
        scanf("%d", &num);
    }
    sprintf(buf+1, "%d", num);
    send(client_sock, buf, strlen(buf), 0);     // send option information
    recv(client_sock, buf, BUF_SIZE, 0);        // recv response
    memset(buf, 0, BUF_SIZE);                   // clear buffer

    // recv msg from server
    while((recv_byte = recv(client_sock, buf, RECV_SIZE, 0)) > 0){
        receive_byte += recv_byte;              // total bytes received
        separate_num++;                         // separation
        // checksum calculation
        if(option == 'C'){
            receive_num += strlen(buf);
            checksum = checksum_C(buf, checksum);
        }
        else{
            receive_num += count_num(recv_byte);
            checksum = checksum_N(buf, recv_byte, checksum);
        }
        memset(buf, 0, BUF_SIZE);               // clear buffer
        delay();
    }
    printf("> total # of %s: %d\n", option=='C'?"letters":"integers", receive_num);
    printf("> total # of transmissions: %d\n", separate_num);
    printf("> total # of bytes: %d\n", receive_byte);
    printf("> checksum: %d\n", checksum);
    putchar('\n');
    printf("Another request? [y/N]: ");
    fflush(stdout);
    getchar();
    scanf("%c", &option);
    close(client_sock);
    if(option == 'Y' || option == 'y')
        goto CONNECT;
    return 0;
}
