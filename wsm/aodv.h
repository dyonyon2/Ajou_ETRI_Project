#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stddef.h>
#include <errno.h>
#include <arpa/inet.h>

//netlink

	#define NETLINK_USER 28
	#define MAX_PAYLOAD 2048 /* maximum payload size*/

//aodv
	#define LL_ADDR_SIZE 6
	#define NMSLOGINFO_SIZE 11
	#define NMS_TABLE_SIZE	118
	#define RREP_ACK       0x1
	#define RREP_REPAIR    0x2
	#define ALLOWED_HELLO_LOSS      2
	#define IFNAMESIZ 16
	#define MAX_NR_INTERFACES 10
	#define MAX_IFINDEX (MAX_NR_INTERFACES - 1)
	#define List_empty(head) ((head) == (head)->next)
	#define List_unattached(le) ((le)->next == NULL && (le)->prev == NULL)
	#define LIST_NULL -1
	#define LIST_SUCCESS 1
	#define List_empty(head) ((head) == (head)->next)
	#define HELLO_INTERVAL          2000
	#define SCH_HELLO_INTERVAL          1000
	#define ALM_INTERVAL		1000
	#define NMS_ALM_INTERVAL 	5000
	#define RANN_INTERVAL           5000
	#define DATA_INTERVAL          10000 //add for data count reset. 
	#define HELLO_TIMEOUT_INTERVAL 5000 
	#define PERR_RANN_MECHANISM_INTERVAL	20000
	#define API_RESET_INTERVAL		1000
	#define List_foreach(curr, head) \
        for (curr = (head)->next; curr != (head); curr = curr->next)

	#define INIT_LIST_ELM(le) do { \
		(le)->next = NULL; (le)->prev = NULL; \
	} while (0)

	#define LIST(name) list_t name = { &(name), &(name) }	
	#define List_foreach_safe(pos, tmp, head) \
        for (pos = (head)->next, tmp = pos->next; pos != (head); \
                pos = tmp, tmp = pos->next)
	#define List_first(head) ((head)->next)
	#define INIT_LIST_HEAD(h) do { \
		(h)->next = (h); (h)->prev = (h); \
	} while (0)

	#define ROUTE_TIMEOUT_SLACK 100

	//6 byte
	typedef struct {
	    uint8_t addr[6];
	} ll_addr;

	
	typedef struct {
	    uint8_t addr[6];
	} AODV_ext;

	//28 Bytes인데 sizeof 하면 28 Bytes
	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t hcnt;			//Hop count
	    uint8_t dest_addr[6];		//ROOT
	    uint8_t orig_addr[6];		//PREQ Sender
	    uint8_t ttl;			//Time to live
		uint8_t r_ch;			//receiving channel number
	    uint32_t seqno;			//Seqno
	    uint32_t orig_IP;			//PREQ Sender IP Address
	    uint32_t alm;			//ALM
	} PREQ;

	//24 Bytes
	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t hcnt;			//Hop count
	    uint8_t dest_addr[6];		//PREQ Sender
	    uint32_t dest_IP;			//PREQ Sender IP Address
	    uint8_t orig_addr[6];		//ROOT
    	uint8_t ext_length;			//Extension length
	    uint32_t orig_IP;			//ROOT IP Address	
		uint8_t seqno;
	} PREP;

	//24 Bytes
	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t orig_addr[6];		//Root Mac Address
	    uint32_t order;			//Rann seq num
	    uint8_t ttl;			//Time to live
		uint8_t metric;
		uint8_t data_rate;
		uint8_t recv_channel;
		uint8_t reset;
		uint8_t log;
		uint8_t update_period;
		uint8_t rann_id;
		ll_addr reset_addr;
	} RANN;

	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t orig_addr[6];		//
		uint8_t next_addr[6];		//
		uint32_t alm_next_hop;
	} RANN_ACK;

	//12 Bytes인데 sizeof 하면 12 Bytes	
	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t r_ch;			//Hop count
	    uint8_t orig_addr[6];		//Hello Sender Mac Address
	    uint32_t orig_IP;	
	} HELLO;

	typedef struct {
	    uint8_t type;			//Msg type
	    unsigned int size;			//size
    } DATA;

	//13 Bytes인데 sizeof 하면 16 Bytes		need to sub 3
	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t timeout_addr[6];		//Timeout MK Mac Address
	    uint8_t dest_addr[6];		//Dest Mac Address
	} PERR;

	typedef struct {
	    uint8_t type;			//Msg type
	    uint8_t timeout_addr[6];		//Timeout MK Mac Address
	    uint8_t orig_addr[6];		//Dest Mac Address
	} PERR_UNI;

	//11 Bytes인데 sizeof 하면 12 Bytes		need to sub 1
	typedef struct {
		uint8_t type;			//Msg type
		uint8_t orig_addr[6];
		uint32_t size;			//Size of log
	} NMSLOGINFO;

	typedef struct {
	    uint8_t type;			//Msg type
	} AODV_msg;

	typedef struct {
		uint8_t type;
		uint8_t value;
	} NMSCONTROLHDR;

	typedef struct {
		uint8_t type;
		uint8_t metric;
		uint8_t data_rate;
		uint8_t recv_channel;
	} NMSCONTROL;

	struct TMP{
	    bool used;				//Used value
	    PREQ t_preq;			//PREQ temp
	    AODV_ext t_ext[5];			//Extension temp
	};

	#define INIT_LIST_ELM(le) do { \
		(le)->next = NULL; (le)->prev = NULL; \
	} while (0)

	typedef void (*timeout_func_t) (void *);

	typedef struct list_t {
	    struct list_t *prev, *next;
	} list_t;

	#define LIST(name) list_t name = { &(name), &(name) }	

	static LIST(TQ);
	
	
	struct timer {
	    list_t l;
	    int used;
	    struct timeval timeout;
	    timeout_func_t handler;
	    void *data;
	};

	struct dev_info {
	    int enabled;		/* 1 if struct is used, else 0 */
	    int sock;			/* AODV socket associated with this device */
	#ifdef CONFIG_GATEWAY
	    int psock;			/* Socket to send buffered data packets. */
	#endif
	    unsigned int ifindex;
	    char ifname[IFNAMSIZ];
	    ll_addr addr;	/* The local IP address */
	    ll_addr broadcast;
	};

	struct host_info {
	    uint32_t seqno;		/* Sequence number */
	    struct timeval bcast_time;	/* The time of the last broadcast msg sent */
	    struct timeval fwd_time;	/* The time a data packet was last forwarded */
	    uint32_t preq_id;		/* RREQ id */
	    int nif;			/* Number of interfaces to broadcast on */
		struct dev_info devs[MAX_NR_INTERFACES+1];
	};

	struct host_info this_host;
	unsigned int dev_indices[MAX_NR_INTERFACES];
	static inline unsigned int ifindex2devindex(unsigned int ifindex)
	{
	    int i;

	    for (i = 0; i < this_host.nif; i++)
		if (dev_indices[i] == ifindex)
		    return i;

	    return MAX_NR_INTERFACES;
	}

	static inline struct dev_info *devfromsock(int sock)
	{
	    int i;

	    for (i = 0; i < this_host.nif; i++) {
		if (this_host.devs[i].sock == sock)
		    return &this_host.devs[i];
	    }
	    return NULL;
	}

	static inline int name2index(char *name)
	{
	    int i;

	    for (i = 0; i < this_host.nif; i++)
		if (strcmp(name, this_host.devs[i].ifname) == 0)
		    return this_host.devs[i].ifindex;

	    return -1;
	}

//

//routing_table

	#define FIRST_PREC(h) ((precursor_t *)((h).next))
	#define seqno_incr(s) ((s == 0) ? 0 : ((s == 0xFFFFFFFF) ? s = 1 : s++))
	#define max(A,B) ( (A) > (B) ? (A):(B))
	#define ACTIVE_ROUTE_TIMEOUT 3000
	#define DELETE_PERIOD 15000

	typedef uint8_t hash_value;	/* A hash value */

	/* Route table entries */
	// Destination, Next hop, Source, Prev

	//	6 + 4 + 6 + 6 + 4 + 4 + 6 + 1 + 1 + 4 + 4 
	struct rt_table {
	    ll_addr dest_addr;	/* IP address of the destination */
	    uint32_t dest_IP;	
		ll_addr source_addr;
		ll_addr prev_addr;
	    uint32_t seqno;
	    unsigned int ifindex;	/* Network interface index... */
	    ll_addr next_hop;	/* IP address of the next hop to the dest */
	    uint8_t hcnt;		/* Distance (in hops) to the destination */
	    uint8_t state;		/* The state of this entry */
		struct timeval first_hello_time;
	    struct timeval last_hello_time;
	    uint32_t hello_cnt;
	    uint32_t alm;
		uint32_t alm_next_hop;
		uint32_t sch_hello_cnt;
		uint8_t alm_buffer[10];
		uint8_t alm_buffer_pointer;
		uint8_t sch_hello_detector;
	};
	
	struct rt_table_list{
		struct rt_table rt_t[10];
	};

	typedef struct IPHeader_tag
	{
		uint32_t SrcIP;
		uint32_t DstIP;	
	}tIPH;

	/* Route entry flags */
	#define RT_UNIDIR        0x1
	#define RT_REPAIR        0x2
	#define RT_INV_SEQNO     0x4
	#define RT_INET_DEST     0x8	/* Mark for Internet destinations (to be relayed
					 * through a Internet gateway. */
	#define RT_GATEWAY       0x10

	/* Route entry states */
	#define INVALID   0
	#define VALID     1

	#define RT_TABLESIZE 64		/* Must be a power of 2 */
	#define RT_TABLEMASK (RT_TABLESIZE - 1)

	
// NMS Data

	struct nms_table{				//						=> 총 118 byte
	    uint8_t addr[6];			//Node addr				=> 6byte
	    uint8_t receiving_channel;	//Receiving channel		=> 1byte
		uint8_t node_state;			//						=> 1byte
		uint8_t metric;				//						=> 1byte
		uint8_t data_rate;			//						=> 1byte
		ll_addr neighbor_node[8];	//Neighbor node id		=> 48byte
		ll_addr uplink_path[8];		//						=> 48byte
		uint32_t alm_next_hop;				//						=> 4byte
		struct timeval last_rann_ack_time;	//8byte			//이거는 NMS로 전송하지 않음 => 마지막 수신 Rann_Ack을 통해 오래동안 Rann_ack 이 오지 않은 노드의 Link quality 예측
	};														//노드가 죽었을 때, 해당 노드에 대한 Rann_ACK을 받을수 없기에 이렇게 사용 

	struct nms_table_list{			//						=> nms_table이 12개 => 12 * 118 byte = 1,248 byte
		struct nms_table nms_t[12];							
	};

/*------------------------------------------------------ */
/*   TYPES                                               */
/*-------------------------------------------------------*/

extern struct rt_table_list *p_rt_list; 
extern ll_addr root_addr;
extern tIPH *pIPH;
extern uint8_t dest_mac_addr[6];
extern uint8_t src_mac_addr[6];
extern uint8_t config_time;
extern bool config_end;
extern bool command_on;
extern uint32_t rann_order;
extern NMSLOGINFO *p_nms_log_str; 
extern struct nms_table_list *p_nms_list;
extern char nms_send_buff[2048];
extern char nms_recv_buff[2048];
extern bool nms_log_on;


/*------------------------------------------------------ */
/*    FUNCTIOS                                           */
/*-------------------------------------------------------*/

void Timer_Reset(void);
void IP_Set(uint32_t t_ip);
int IP_Compare(uint32_t lo_ip);

int Listelm_detach(list_t * prev, list_t * next);
int Listelm_add(list_t * le, list_t * prev, list_t * next);
int List_add_tail(list_t * head, list_t * le);
int List_add(list_t * head, list_t * le);
int List_detach(list_t * le);

long Timeval_diff(struct timeval *t1, struct timeval *t2);
int Timer_remove(struct timer *t);
void Timer_add(struct timer *t);
void Timer_set_timeout(struct timer *t, long msec);
int Timer_init(struct timer *t, timeout_func_t f, void *data);

void NMS_logging(unsigned int size, uint8_t * msg);
void NMS_log_add(unsigned int size, uint8_t * msg);
void NMS_log_send();
void NMS_log_reset();
void NMS_log_forward(uint8_t *buf, unsigned int size, ll_addr source);
void NMS_log_process(uint8_t * pPayload);
void NMS_log_send_nms(NMSLOGINFO *str, uint8_t *msg,unsigned int size);


void Hello_set(HELLO *hello, uint8_t hcnt, uint8_t *Src_addr,  uint32_t ip4);
void Rann_set(RANN *rann, uint8_t *Src_addr);
void Preq_set(PREQ *preq, uint8_t *Dst_addr, uint8_t *Src_addr, uint32_t order, uint32_t r_ch);
void Prep_set(PREP *prep, uint8_t hcnt, uint8_t *Dst_addr, uint8_t *Src_addr);
void Perr_set(PERR *perr, uint8_t *timeout_addr, uint8_t *dest_addr);

void Make_msg_format_HELLO(void);
void Make_msg_format_RANN(ll_addr *reset_addr);
void Make_msg_format_PREQ(ll_addr *route, uint32_t order, uint32_t r_ch);
void Make_msg_format_PREP(PREQ * preq);
void Make_msg_format_DATA(ll_addr *route, unsigned int size, char* buf);
void Make_msg_format_PERR(ll_addr *timeout_addr,ll_addr *dest_addr);
void Make_msg_format_PERR_unicast(ll_addr *timeout_node);

void alm_start();
void alm_calculator();
void sch_hello_check();
void nms_alm_calculator();

void Hello_send(void);
void Hello_start(void);
void SCH_Hello_start();

void Hello_timeout_process(void);
void Hello_timeout_start(void);

void Rann_forward(RANN *pre);
void Rann_send(void);
void Rann_start(void);
void Preq_send(ll_addr *route, uint32_t order, uint32_t r_ch);
void Preq_forward(PREQ *p_pre);

void Prep_send(PREQ *preq);
void Prep_forward(PREP *pre);

void Data_send_dst(char *msg,unsigned int size);
void Data_forward(char *buf,uint32_t DstIP, unsigned int size);
void Data_start(char *msg, unsigned int size);
void Data_start2(ll_addr *route, char *msg, int size);
void Data_send_nms(char *msg,unsigned int size);

void Data_start3(ll_addr *route, char* msg, int size);
void PREP_TEST_CCH(ll_addr *route);
void PREP_TEST_SCH(ll_addr *route);

void Perr_send(ll_addr *timeout, ll_addr *dest);
void Perr_unicast(ll_addr *timeout_node);
void Perr_forward(PERR_UNI *per);


void API_reset_process(void);
void API_reset_timer_start(void);

void Receive_process(uint8_t *pPayload);
void Hello_process(uint8_t * pBuf);
void Rann_process(uint8_t * pBuf);
void Preq_process(uint8_t * pBuf);
void Prep_process(uint8_t * pBuf);
void Data_process(uint8_t * pBuf);
void Data_process2(uint8_t * pBuf);
void Perr_process(uint8_t * pBuf);
void Perr_rann_mechanism(ll_addr timeout_addr);
void Perr_unicast_state_process(uint8_t * pPayload);
void Perr_unicast_process(uint8_t * pPayload);
void nms_control_msg_process(char *control_msg);

void rt_NMS_reset(void);
void rt_table_empty(int index);
void rt_table_init(struct rt_table_list *p_rt_list);
struct rt_table *rt_table_find(ll_addr dest_addr, uint8_t source_consider ,ll_addr source_addr);
void rt_table_update(struct rt_table *rt, ll_addr next, uint8_t hcnt, struct timeval *now,uint32_t alm, uint32_t seqno);
int rt_table_get_index(ll_addr dest_addr);
int rt_table_insert(ll_addr dest_addr, ll_addr next, ll_addr source_addr, uint8_t prev_exist, ll_addr prev_addr, uint8_t hcnt, uint32_t IPaddr, uint32_t hello_cnt, struct timeval *now, uint32_t alm, uint32_t seqno);
struct rt_table *rt_table_find_IP(uint32_t IPaddr);
void rt_table_print(void);

void nms_table_empty(int index);
struct nms_table* nms_table_find(ll_addr addr);
void nms_table_perr_update(ll_addr timeout_addr, ll_addr orig_addr);
int nms_table_update(struct nms_table* nms, ll_addr neighbor_addr, uint8_t r_ch, uint8_t state, uint8_t is_hello);
int nms_table_insert(ll_addr addr, ll_addr neighbor, uint8_t r_ch, uint8_t state, uint8_t is_hello);
int nms_table_uplink_insert(struct nms_table* nms, AODV_ext* ext, uint8_t ext_length);

void nms_table_print(void);
void Timer_timeout(struct timeval *now);
