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
    char send_msg[1000]="This is client!!!!";
    int size = -1;
    uint32_t value=0;

    int i = 0, cnt = 0, j=0;

    clnt_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(clnt_sock == -1)
        error_handling("socket error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(clnt_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect error");
    
    while(1){

        size=read(clnt_sock,recv_msg, 1000);
        cnt++;

        if(size==-1)
            error_handling("read error");
        else{

            printf("\n\n-------------------------- cnt %d / recv %d-----------------------------\n",cnt,size);
            for(i=0;i<110;i++)
                printf("%02x ",recv_msg[i]);
            
            memcpy(&value,&recv_msg[106],4);
            printf("value : %u \n",value);
            // recv_msg[109]
        }
    }

    close(clnt_sock);

    return 0;
}

void error_handling(char *msg){
    fputs(msg, stderr);
    fputc('\n',stderr);
    exit(1);
}

