/* PC1 side UDP Tx implementation */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#define HOSTNAME "192.168.50.128"
#define PORT     4200

int main(void)
{
    char msg[50];
    struct sockaddr_in servaddr;
    int fd = socket(AF_INET,SOCK_DGRAM,0);

    if(fd<0){
        perror("cannot open socket");
        return 0;
    }

    strcpy(msg, "Hello World!");

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(HOSTNAME);
    servaddr.sin_port = htons(PORT);
    if (sendto(fd, msg, strlen(msg)+1, 0, // +1 to include terminator
               (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("cannot send message");
        close(fd);
        return 0;
    }
    close(fd);
    return 0;
}
