#ifndef _MASH_EXTERN_H_
#define _MASH_EXTERN_H_

#define IPv4_ADDR_STR_LEN       16  /* string length of IPv4 address including null */
#define _ETHER_ADDR_LEN         6    /* length of an Ethernet address */

#define MAX_QUEUE	20     /* 1sec 10 / over 500byte send *2 */
#define MAX_FRAME_LEN   2048

/* external variables */
extern uint32_t RootNode;        /* is root node ? */
extern uint32_t metric;
extern uint8_t receiving_channel;
extern uint8_t data_rate;
extern uint8_t route_reset;
extern uint8_t api_reset;
extern uint8_t NMS_Connection;
extern uint8_t rann_id;
extern int nms_tcp_send_client_ext;
extern uint8_t reboot_flag;
extern float update_cycle;

extern uint32_t my_ip;
extern uint8_t my_mac[6];
extern uint8_t print_time;

typedef struct {
	uint8_t buff[MAX_QUEUE][MAX_FRAME_LEN];
	uint32_t len[MAX_QUEUE];
	int front;
	int rear;
	uint8_t unicast[MAX_QUEUE];
	uint8_t if_num[MAX_QUEUE]; //DEFAULT_IF_IDX = 0 > for use two channel. //IF0_CHAN_NUM, IF1_CHAN_NUM
	//uint8_t dest[MAX_QUEUE][6]
} tx_queue;


extern tx_queue tq;
/*
 * RootNode_Set - setter of RootNode variable.
 */
static inline void RootNode_Set(uint32_t node_value)
{
    if (node_value < 2)
    {
        /* set valid value */
        RootNode = node_value;
    }
    else
    {
        /* not set */
    }
}

/*
 * RootNode_Get - getter of RootNode variable.
 */
static inline uint32_t RootNode_Get(void)
{
    return (RootNode);
}

#ifdef __cplusplus
extern "C" {
#endif
void print_d(char *ptr, int size);

int mesh_get_ipv4_address(const char *ifname, char *address);
int mesh_get_mac_address(const char *ifname, uint8_t *mac_addr);


void Init_Q(void);
int  IsEmpty(void);
void addq(uint32_t len, uint8_t *pBuf, uint8_t uni_type, uint8_t if_chan);
void deleteq(void);
void printq( int first);
#ifdef __cplusplus
}
#endif

#endif /* !_MASH_EXTERN_H_*/
