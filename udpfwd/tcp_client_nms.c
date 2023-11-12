#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/netlink.h>
#include <net/ethernet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#include "aodv.h"

struct nms_table_list p_nms_list;

void error_handling(char *msg);
void nms_table_print(void);

int main(int argc, char* argv[]){
    int clnt_sock;
    struct sockaddr_in serv_addr;
    char recv_msg[200];
    char send_msg[100]="This is client!!!!";

    int i =0;

    struct nms_table_list nms_list;
    p_nms_list = &nms_list;
    memset(&nms_list, 0, sizeof(struct nms_table_list));

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

        for(i = 0; i<12 ; i++){
            memset(recv_msg,0,200);
            if(recv(clnt_sock,recv_msg, sizeof(recv_msg)-1,0) == -1)
                error_handling("read error");
            memcpy(&p_nms_list->nms_t[i],)

        }
        


        printf(" server message : %s\n",recv_msg);

        send(clnt_sock,send_msg,strlen(send_msg),0);

    }
    // send(serv_addr,send_msg,strlen(send_msg));

    close(clnt_sock);

    return 0;
}

void error_handling(char *msg){
    fputs(msg, stderr);
    fputc('\n',stderr);
    exit(1);
}

void nms_table_print(void)
{
	int i = 0, cnt = 0, j=0;
	printf("\n------------------NMS Table Print--------------------\n");
	printf(" Index	source				receiving_channel		node_state\n");
	printf(" neighbor_node\n");
	printf(" uplink_path\n");
	printf("-------------------------------------------------------\n");
	for(i = 0;i < 11;i++){
		if(p_nms_list->nms_t[i].addr[0]==0 && p_nms_list->nms_t[i].addr[1]==0 && p_nms_list->nms_t[i].addr[3]==0 && p_nms_list->nms_t[i].addr[4]==0 && p_nms_list->nms_t[i].addr[5]==0 )
			continue;
		cnt ++;
		printf("\n %d	%02x.%02x.%02x.%02x.%02x.%02x		%d				%d\n",cnt ,p_nms_list->nms_t[i].addr[0],p_nms_list->nms_t[i].addr[1],p_nms_list->nms_t[i].addr[2],p_nms_list->nms_t[i].addr[3],p_nms_list->nms_t[i].addr[4],p_nms_list->nms_t[i].addr[5],p_nms_list->nms_t[i].receiveing_channel,p_nms_list->nms_t[i].node_state);
		for(j=0;j<8;j++){
			if(p_nms_list->nms_t[i].neighbor_node[j].addr[0]==0 && p_nms_list->nms_t[i].neighbor_node[j].addr[1]==0 && p_nms_list->nms_t[i].neighbor_node[j].addr[2]==0 && p_nms_list->nms_t[i].neighbor_node[j].addr[3]==0 && p_nms_list->nms_t[i].neighbor_node[j].addr[4]==0 && p_nms_list->nms_t[i].neighbor_node[j].addr[5]==0)
				break;
			printf(" 	neihbor_node %d :",j+1);
			printf(" %02x.%02x.%02x.%02x.%02x.%02x\n",p_nms_list->nms_t[i].neighbor_node[j].addr[0],p_nms_list->nms_t[i].neighbor_node[j].addr[1],p_nms_list->nms_t[i].neighbor_node[j].addr[2],p_nms_list->nms_t[i].neighbor_node[j].addr[3],p_nms_list->nms_t[i].neighbor_node[j].addr[4],p_nms_list->nms_t[i].neighbor_node[j].addr[5]);
		}
		for(j=0;j<8;j++){
			if(p_nms_list->nms_t[i].uplink_path[i].addr[0]==0 && p_nms_list->nms_t[i].uplink_path[j].addr[1]==0 && p_nms_list->nms_t[i].uplink_path[j].addr[2]==0 && p_nms_list->nms_t[i].uplink_path[j].addr[3]==0 && p_nms_list->nms_t[i].uplink_path[j].addr[4]==0 && p_nms_list->nms_t[i].uplink_path[j].addr[5]==0)
				break;
			printf(" 	uplink_path %d :",j+1);
			printf("%02x.%02x.%02x.%02x.%02x.%02x\n",p_nms_list->nms_t[i].uplink_path[j].addr[0],p_nms_list->nms_t[i].uplink_path[j].addr[1],p_nms_list->nms_t[i].uplink_path[j].addr[2],p_nms_list->nms_t[i].uplink_path[j].addr[3],p_nms_list->nms_t[i].uplink_path[j].addr[4],p_nms_list->nms_t[i].uplink_path[j].addr[5]);
		}
	}
	printf("--------------------------------------------------------\n");
	printf(" TOTAL :                                  %d           \n", cnt);
	printf("--------------------------------------------------------\n\n");
}
