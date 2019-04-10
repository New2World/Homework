/*
* CISC650 - Network - Project I
* author: Ruiqi Wang
* date: 3/13/2019
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>     // for socket, connect, send and recv
#include <netinet/in.h>     // for sockaddr_in

#define BUF_SIZE 1000

int checksum = 0;

int fill_buf(char *buf, char c, int total){
    /*
    generate 100 characters or 100 16-bit integers to fill the buffer
    */
    int i = 0;
    int cnt;
    unsigned short int num[50] = {0};
    memset(buf, 0, BUF_SIZE);
    if(c == 'C'){
        cnt = total>100?100:total;
        for(;i < cnt;i++){
            buf[i] = rand()%('z'-'a')+'a';
            checksum += buf[i];
            checksum %= 256;
        }
        buf[cnt] = 0;
    }
    else{
        cnt = total>50?50:total;
        for(;i < cnt;i++){
            num[i] = rand()%65536;
            checksum += num[i];
            checksum %= 65536;
        }
        memcpy(buf, num, 2*cnt);
        buf[2*cnt] = 0;
    }
    return cnt;
}

void delay(){
    /*
    delay, wait for transmission finished
    */
    int delay_num = (rand()%900+100)*1000;
    while(delay_num--);
}

int main(int argc, char **argv){
    int server_sock, client_sock;
    int server_port = 12000;
    uint sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char buf[BUF_SIZE] = {0};

    char option = buf[0];
    int total_bytes = 0;
    int total_num = atoi(buf+1);
    int tobe_transmit = total_num;
    int separate_num = 0;
    int generate_num = 0;

    srand(time(NULL));
    if(argc > 1)
        server_port = atoi(argv[1]);
    // initialize server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);
    printf("Server is listening on port %d\n\n", server_port);
    // create socket
    if((server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation");
        return 1;
    }
    // bind socket with server_addr
    if(bind(server_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("binding");
        return 1;
    }
    LISTEN:
    // listen on port
    listen(server_sock, 5);
    // recv connection request and accept connection
    if((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &sin_size)) < 0){
        perror("acception");
        return 1;
    }
    memset(buf, 0, BUF_SIZE);                       // clear buffer
    recv(client_sock, buf, BUF_SIZE, 0);            // recv option information
    send(client_sock, buf, strlen(buf), 0);         // send confirm
    // clear variables to be used
    option = buf[0];
    checksum = 0;
    total_bytes = 0;
    separate_num = 0;
    total_num = atoi(buf+1);
    tobe_transmit = total_num;
    // network delay
    delay();
    // send msg
    while(tobe_transmit > 0){
        // generate characters or numbers following request
        generate_num = fill_buf(buf, option, tobe_transmit);
        tobe_transmit -= generate_num;
        separate_num++;
        // send to client
        total_bytes += send(client_sock, buf, option=='C'?generate_num:2*generate_num, 0);
        delay();
    }
    printf("> total # of %s: %d\n", option=='C'?"letters":"integers", total_num);
    printf("> total # of transmissions: %d\n", separate_num);
    printf("> total # of bytes: %d\n", total_bytes);
    printf("> checksum: %d\n", checksum);
    putchar('\n');
    close(client_sock);
    goto LISTEN;

    close(server_sock);

    return 0;
}
