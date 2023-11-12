#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *msg);

int main(int argc, char* argv[]){
    int clnt_sock;
    struct sockaddr_in serv_addr;
    char recv_msg[100000];
    char send_msg[1000]="";
    int size = -1;
    uint8_t metric = 1;
    uint8_t data_rate = 12;
    uint8_t recv_channel = 172;
    uint8_t reset = 0;

    int i = 0, cnt = 0, j=0;

    clnt_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(clnt_sock == -1)
        error_handling("socket error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    memcpy(&send_msg[0],&metric,1);
    memcpy(&send_msg[1],&data_rate,1);
    memcpy(&send_msg[2],&recv_channel,1);
    memcpy(&send_msg[3],&reset,1);

    if(connect(clnt_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect error");
    
    size=send(clnt_sock,send_msg, 4,0);
    cnt++;

    if(size==-1)
        error_handling("read error");
    else{

        printf("\n\n-------------------------- send ! -----------------------------\n");
    }
    printf("\n%u\n",metric);
    printf("\n%u\n",data_rate);
    printf("\n%u\n",recv_channel);
    printf("\n%u\n",reset);

    close(clnt_sock);

    return 0;
}

void error_handling(char *msg){
    fputs(msg, stderr);
    fputc('\n',stderr);
    exit(1);
}

