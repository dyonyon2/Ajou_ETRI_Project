/* PC2 RX side implementation */
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdint.h>
#include <stdbool.h>


#include "aodv.h"

#define PORT    8888
#define MAXLINE 2048
  
// Driver code 
int main(void) 
{ 
    int sockfd; 
    uint8_t buffer[MAXLINE]=""; 
    struct sockaddr_in servaddr, cliaddr; 
    int broadcast=1;
    int result;
    char copy_buf[100]="";

    FILE *fp = fopen("log.txt","w");
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    result = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
    
    if (result < 0)
    {
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    // Filling server information 
    servaddr.sin_family      = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port        = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    int len, n,i,size; 
    char bit;
  
    len = sizeof(cliaddr);  //len is value/resuslt 
  
    while(1)
    {
        n = recvfrom(sockfd, buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
        printf("UDP recive n = %d\n", n);     
        // printf("contents = %d\n", buffer);   
        memcpy(&copy_buf[0],buffer,n);  

        uint8_t m_type = ((AODV_msg *)(buffer+sizeof(NMSLOGINFO)))->type;

        NMSLOGINFO *nms_str;
        AODV_msg* aodv_msg;
        RANN* rann_msg;	
        PREQ* preq_msg;	
        PREP* prep_msg;	
        PERR* perr_msg;	

        nms_str = (NMSLOGINFO *)buffer;
        aodv_msg = (AODV_msg *) (buffer+sizeof(NMSLOGINFO));

        // for(i=0;i<32;i++)
        //     printf("%u ",buffer[i]);
        // printf("contents : %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11],buffer[12],buffer[13],buffer[14]); 

        printf("nmsg_str->size : %u\n",nms_str->size);
        printf("nmsg_str->type : %u\n",nms_str->type);
        printf("nmsg_str->addr : %02x:%02x:%02x:%02x:%02x:%02x\n",nms_str->orig_addr[0],nms_str->orig_addr[1],nms_str->orig_addr[2],nms_str->orig_addr[3],nms_str->orig_addr[4],nms_str->orig_addr[5]);

        switch(m_type)
        {
            case 1: //rann
                rann_msg = (RANN *) (buffer+sizeof(NMSLOGINFO));
                printf("RANN msg->type = %u\n",rann_msg->type);
                printf("RANN msg->addr= %02x:%02x:%02x:%02x:%02x:%02x\n",rann_msg->orig_addr[0],rann_msg->orig_addr[1],rann_msg->orig_addr[2],rann_msg->orig_addr[3],rann_msg->orig_addr[4],rann_msg->orig_addr[5]);
                printf("RANN msg->order = %u\n",rann_msg->order);
                printf("RANN msg->ttl = %u\n",rann_msg->ttl);

            break;

            case 2: //preq
                preq_msg = (PREQ *) (buffer+sizeof(NMSLOGINFO));		
                printf("PREQ msg->type = %u\n",preq_msg->type);
                printf("PREQ msg->hcnt = %u\n",preq_msg->hcnt);
                printf("PREQ msg->orin_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",preq_msg->orig_addr[0],preq_msg->orig_addr[1],preq_msg->orig_addr[2],preq_msg->orig_addr[3],preq_msg->orig_addr[4],preq_msg->orig_addr[5]);
                printf("PREQ msg->dest_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",preq_msg->dest_addr[0],preq_msg->dest_addr[1],preq_msg->dest_addr[2],preq_msg->dest_addr[3],preq_msg->dest_addr[4],preq_msg->dest_addr[5]);
                printf("PREQ msg->ttl = %u\n",preq_msg->ttl);
                printf("PREQ msg->r_ch = %u\n",preq_msg->r_ch);
                printf("PREQ msg->orig_IP = %u\n",preq_msg->orig_IP);
                printf("PREQ msg->alm = %u\n",preq_msg->alm);
            break;

            case 3: //prep
                prep_msg = (PREP *) (buffer+sizeof(NMSLOGINFO));			
                printf("PREP msg->type = %u\n",prep_msg->type);
                printf("PREP msg->hcnt = %u\n",prep_msg->hcnt);
                printf("PREP msg->orig_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",prep_msg->orig_addr[0],prep_msg->orig_addr[1],prep_msg->orig_addr[2],prep_msg->orig_addr[3],prep_msg->orig_addr[4],prep_msg->orig_addr[5]);
                printf("PREP msg->orig_IP = %u\n",prep_msg->orig_IP);
                printf("PREP msg->dest_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",prep_msg->dest_addr[0],prep_msg->dest_addr[1],prep_msg->dest_addr[2],prep_msg->dest_addr[3],prep_msg->dest_addr[4],prep_msg->dest_addr[5]);
                printf("PREP msg->dest_IP = %u\n",prep_msg->dest_IP);
                printf("PREP msg->ext_length = %u\n",prep_msg->ext_length);
            break;

            case 5: //perr
                perr_msg = (PERR *) (buffer+sizeof(NMSLOGINFO));
                printf("PERR msg->type = %u\n",perr_msg->type);
                printf("PERR msg->timeout_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",perr_msg->timeout_addr[0],perr_msg->timeout_addr[1],perr_msg->timeout_addr[2],perr_msg->timeout_addr[3],perr_msg->timeout_addr[4],perr_msg->timeout_addr[5]);
                printf("PERR msg->dest_addr= %02x:%02x:%02x:%02x:%02x:%02x\n",perr_msg->dest_addr[0],perr_msg->dest_addr[1],perr_msg->dest_addr[2],perr_msg->dest_addr[3],perr_msg->dest_addr[4],perr_msg->dest_addr[5]);
            break;
            
            default:
            break;
        }

        // fwrite(copy_buf,n,&size,fp);
        
        // printf("copy size = %d\n",size);


        // fclose(fp);

    }
    return 0; 
} 
