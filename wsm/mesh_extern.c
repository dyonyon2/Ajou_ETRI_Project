#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "mesh_extern.h"

/* external variables */
uint32_t RootNode;        /* is root node ? */
uint32_t metric = 1;          /* 1-default, hopcount 2-alm metric */
uint8_t receiving_channel = 172;
uint8_t data_rate = 12;
uint8_t route_reset = 0;
uint8_t api_reset = 0;
uint8_t NMS_Connection = 0;
uint8_t rann_id = 0;
int nms_tcp_send_client_ext = 0;
uint8_t reboot_flag = 0;
float update_cycle = 1;



uint32_t my_ip;
uint8_t my_mac[6];
uint8_t print_time = 0; 

tx_queue tq;

void Init_Q(void)
{
	tq.front = -1;
	tq.rear =-1;
	int i = 0, j = 0;

	for(i = 0; i < MAX_QUEUE; i ++){
		for (j = 0; j < MAX_FRAME_LEN; j ++)
			tq.buff[i][j] = 0;
		tq.len[i] = 0;
		tq.unicast[i] = 0;
		tq.if_num[i] = 0;
	}
	 printf("Init %d size Queue \n",MAX_FRAME_LEN );
}

int IsEmpty(void){
    if(tq.front == tq.rear)//front�� rear�� ������ ť�� ����ִ� ���� 
        return 1;
    else return 0;
}
int IsFull(void){
    int tmp = (tq.rear+1)%MAX_QUEUE; //���� ť���� rear+1�� MAX�� ���� ����������
    if(tmp == tq.front)//front�� ������ ť�� ������ �ִ� ���� 
        return 1;
    else
        return 0;
}
void addq(uint32_t len, uint8_t *pBuf, uint8_t uni_type, uint8_t if_chan)
{

	    if(IsFull()){
		printf("Queue is Full.\n");
	    }
	    else{
		 tq.rear = (tq.rear+1)%MAX_QUEUE;
		 tq.len[tq.rear] = len;
		 memcpy(&tq.buff[tq.rear], pBuf, len);
		 tq.unicast[tq.rear] = uni_type;
		 tq.if_num[tq.rear] = if_chan;
	 	//  printf("addq front : %d, rear: %d\n", tq.front, tq.rear);
	    }
    
}
void printq( int first)
{
	uint32_t i = 0;
	//printf("---- queue unicast(1) %d ----\n", tq.unicast[first]);
 	for( i = 0; i < tq.len[first] ; i++){
		if ((i % 16) == 0)
			printf("S %04x: ", i);
		printf("%02x ", tq.buff[first][i]);

		// end of line
		if (((i + 1) % 16) == 0) printf("\n");	
	    }
    	printf("\n");

}
void deleteq(void){

    if(IsEmpty()) {
        printf("Queue is Empty.\n");
    }
    else{
        tq.front = (tq.front+1)%MAX_QUEUE;
 	//printf("deleteq front : %d, rear: %d\n", tq.front, tq.rear);
    }
}



int mesh_get_ipv4_address
    (
    const char *ifname,     /* [in]  network ineterface name */
    char       *address     /* [out] IPv4 address */
    )
{
    struct ifreq ifr;
    int          fd;
    int          status;

    if ((ifname == NULL) || (address == NULL))
    {
        errno  = EINVAL;
        status = -1;
    }
    else
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (fd < 0)
        {
            status = -1;
        }
        else
        {
            /* set to get IPv4 Address */
            ifr.ifr_addr.sa_family = AF_INET;

            /* set interface name passed from parameter */
            strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

            status = ioctl(fd, SIOCGIFADDR, &ifr);

            if (status != -1)
            {
                /*
                 * copy IPv4 address to output buffer
                 * XXX: inet_ntoa is M.T safe
                 */
                strncpy(address, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), IPv4_ADDR_STR_LEN);
            }
        }
    }

    return (status);
}

void print_d(char *ptr, int size)
{
	int i = 0; 
	for (i = 0; i < (int) size; i ++)
	{
		if( i > 0 && i % 16 == 0) printf("\n");
		printf(" %02x", ptr[i]);
	}

	printf(" total %d size\n", size); 


}

int mesh_get_mac_address
    (
    const char *ifname,     /* [in]  network ineterface name */
    uint8_t    *mac_addr    /* [out] MAC address */
    )
{
    struct ifreq ifr;
    int          fd;
    int          status;
    int          index;

    if ((ifname == NULL) || (mac_addr == NULL))
    {
        errno  = EINVAL;
        status = -1;
    }
    else
    {
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

        if (fd < 0)
        {
            status = -1;
        }
        else
        {
            /* set interface name passed from parameter */
            strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

            status = ioctl(fd, SIOCGIFHWADDR, &ifr);

            if (status == 0)
            {
                for (index = 0; index < _ETHER_ADDR_LEN; index++)
                {
                    mac_addr[index] = (uint8_t)ifr.ifr_addr.sa_data[index];
                }
            }
        }
    }

    return (status);

}

#define __MESH_TEST 1

#ifdef __MESH_TEST

#include <stdio.h>

void test_mesh001(void)
{
    char     ipv4_addr[16];  //IPv4_ADDR_STR_LEN
    uint8_t  mac[6];  //_ETHER_ADDR_LEN

    /* clear bufffers */
    memset(ipv4_addr, 0, 16);   //IPv4_ADDR_STR_LEN
    memset(mac, 0, 6);  //_ETHER_ADDR_LEN

    /* get IP address of wave-data0 */
    mesh_get_ipv4_address("eth0", ipv4_addr);

    /* get MAC address of wave-data0 */
    mesh_get_mac_address("eth0", mac);

    printf("eth0 IP: %s, MAC: %hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n,",
        ipv4_addr, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* get IP address of wave-data0 */
    mesh_get_ipv4_address("wave-data0", ipv4_addr);

    /* get MAC address of wave-data0 */
    mesh_get_mac_address("wave-data0", mac);

    printf("wave-data0 IP: %s, MAC: %hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n,",
        ipv4_addr, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#endif
