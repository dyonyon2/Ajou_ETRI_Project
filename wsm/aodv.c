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
#include <sys/reboot.h>
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
#include "mesh_extern.h"


#define NETIFNAME "wave-data0"
#define PORT 8889

ll_addr root_addr;
ll_addr perr_rann_node;
struct rt_table_list *p_rt_list; 
tIPH *pIPH;

uint8_t mac_list[4][6]={
	{0x00, 0xbf, 0xba, 0xa4, 0x44, 0x00},
	{0x00, 0xe0, 0x4c, 0x36, 0x34, 0x20},
	{0x00, 0x63, 0x36, 0x77, 0x9a, 0x00},
	{0x00, 0x36, 0x5b, 0x95, 0xa0, 0x00}
};

uint8_t src_mac_addr[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t dest_mac_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t config_time = 5000;//config_time * hello interval(2sec) = total config time

bool command_on = false;
bool config_end = false;
uint32_t rann_order=0;

static struct timer sch_hello_timer,sch_hello_check_timer,hello_timer,rann_timer, hello_timeout_timer, alm_timer, nms_alm_timer, data_timer, perr_rann_timer, API_reset_timer;
static struct timer route_select_timer[10];

static struct TMP tmp_preq[10];

static NMSLOGINFO nms_log_str;
static uint8_t nms_log_buff[MAX_FRAME_LEN]="";

struct nms_table_list *p_nms_list;

//update 2020.10.19 neuron
static unsigned int timer_cnt =0; //for stop hello/rann/alm timer(send)
static unsigned int data_cnt = 0; //for data forwarding count
static unsigned int b_data_cnt = 0;
static unsigned int perr_rann_check =0;

extern int nms_server_tcp;

void Timer_Reset(void)
{
	int i = 0;
	Timer_remove(&hello_timer);
	Timer_remove(&sch_hello_timer);
	Timer_remove(&sch_hello_check_timer);
	Timer_remove(&rann_timer);
	Timer_remove(&alm_timer);
	Timer_remove(&data_timer);
	Timer_remove(&hello_timeout_timer);
	Timer_remove(&data_timer);
	Timer_remove(&API_reset_timer);

	for(i = 0; i < 10; i++)
		Timer_remove(&route_select_timer[i]);
 	
}
void IP_Set(uint32_t t_ip)
{
	my_ip = t_ip;
	printf("---- IP SET address  s:  %i.%i.%i.%i \n", my_ip & 0xFF, (my_ip >> 8) & 0xFF, (my_ip >> 16) & 0xFF, ( my_ip>>24) & 0xFF);
 
}

int IP_Compare(uint32_t lo_ip)
{
	int i = 0;
	int ret = 0;
	
	if(lo_ip == my_ip) ret = 1;

	return ret;
}

void rt_table_print(void)
{
	int i = 0, cnt = 0;
	printf("\n----------------------Routing Table Print-------------------------\n");
	printf("Index	Destination		Next hop		Source			Prev			IP			hop count	alm(next hop)	alm	sch_hello_count\n");

	if((memcmp(&root_addr.addr, my_mac, 6) != 0))
	printf(" ROOT			   %02x:%02x:%02x:%02x:%02x:%02x\n", root_addr.addr[0],  root_addr.addr[1],  root_addr.addr[2],  root_addr.addr[3],  root_addr.addr[4],  root_addr.addr[5]);
	printf("-------------------------------------------------------\n");
	for(i = 0;i < 11;i++) 
	{
		if(p_rt_list->rt_t[i].state == 1)
		{
			cnt ++;
			printf("%d	%02x.%02x.%02x.%02x.%02x.%02x	%02x.%02x.%02x.%02x.%02x.%02x	%02x.%02x.%02x.%02x.%02x.%02x	%02x.%02x.%02x.%02x.%02x.%02x	%i.%i.%i.%i		%d		%u		%u	%d\n", i, p_rt_list->rt_t[i].dest_addr.addr[0], p_rt_list->rt_t[i].dest_addr.addr[1], p_rt_list->rt_t[i].dest_addr.addr[2], p_rt_list->rt_t[i].dest_addr.addr[3], p_rt_list->rt_t[i].dest_addr.addr[4], p_rt_list->rt_t[i].dest_addr.addr[5], 
				p_rt_list->rt_t[i].next_hop.addr[0], p_rt_list->rt_t[i].next_hop.addr[1], p_rt_list->rt_t[i].next_hop.addr[2], p_rt_list->rt_t[i].next_hop.addr[3], p_rt_list->rt_t[i].next_hop.addr[4], p_rt_list->rt_t[i].next_hop.addr[5], p_rt_list->rt_t[i].source_addr.addr[0], p_rt_list->rt_t[i].source_addr.addr[1], p_rt_list->rt_t[i].source_addr.addr[2], p_rt_list->rt_t[i].source_addr.addr[3], p_rt_list->rt_t[i].source_addr.addr[4], p_rt_list->rt_t[i].source_addr.addr[5], p_rt_list->rt_t[i].prev_addr.addr[0], p_rt_list->rt_t[i].prev_addr.addr[1], p_rt_list->rt_t[i].prev_addr.addr[2], p_rt_list->rt_t[i].prev_addr.addr[3], p_rt_list->rt_t[i].prev_addr.addr[4], p_rt_list->rt_t[i].prev_addr.addr[5], 
				p_rt_list->rt_t[i].dest_IP & 0xFF, (p_rt_list->rt_t[i].dest_IP >> 8) & 0xFF, (p_rt_list->rt_t[i].dest_IP>> 16) & 0xFF, ( p_rt_list->rt_t[i].dest_IP >>24) & 0xFF ,p_rt_list->rt_t[i].hcnt,p_rt_list->rt_t[i].alm_next_hop,p_rt_list->rt_t[i].alm,p_rt_list->rt_t[i].sch_hello_cnt);

		}
	}
	printf("--------------------------------------------------------\n");
	printf(" TOTAL :                                  %d           \n", cnt);
	printf("--------------------------------------------------------\n\n");

}
unsigned short csum(unsigned short *buf, int nwords)
{
  unsigned long sum;
  for(sum=0; nwords>0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum &0xffff);
  sum += (sum >> 16);
  return (unsigned short)(~sum);
}

int Listelm_detach(list_t * prev, list_t * next)
{
    next->prev = prev;
    prev->next = next;

    return LIST_SUCCESS;
}

int Listelm_add(list_t * le, list_t * prev, list_t * next)
{
    prev->next = le;
    le->prev = prev;
    le->next = next;
    next->prev = le;

    return LIST_SUCCESS;
}

int List_add_tail(list_t * head, list_t * le)
{

    if (!head || !le)
	return LIST_NULL;

    Listelm_add(le, head->prev, head);

    return LIST_SUCCESS;
}

int List_add(list_t * head, list_t * le)
{

    if (!head || !le)
	return LIST_NULL;

    Listelm_add(le, head, head->next);

    return LIST_SUCCESS;
}

int List_detach(list_t * le)
{
    if (!le)
	return LIST_NULL;

    Listelm_detach(le->prev, le->next);

    le->next = le->prev = NULL;

    return LIST_SUCCESS;
}

long Timeval_diff(struct timeval *t1, struct timeval *t2)
{
    long long res;		/* We need this to avoid overflows while calculating... */

    if (!t1 || !t2)
	return -1;
    else {

	res = t1->tv_sec;
	res = ((res - t2->tv_sec) * 1000000 + t1->tv_usec - t2->tv_usec) / 1000;
	return (long) res;
    }
}

int Timer_remove(struct timer *t)
{
    int res = 1;

    if (!t)
	return -1;


    if (List_unattached(&t->l))
	res = 0;
    else
	List_detach(&t->l);

    t->used = 0;

    return res;
}

void Timer_add(struct timer *t)
{
    list_t *pos;

    if (!t) {
	perror("NULL timer!!!\n");
	exit(-1);
    }
    if (!t->handler) {
	perror("NULL handler!!!\n");
	exit(-1);
    }

    if (t->used)
	Timer_remove(t);

    t->used = 1;


    if (List_empty(&TQ)) {
	List_add(&TQ, &t->l);
    } else {

	List_foreach(pos, &TQ) {
	    struct timer *curr = (struct timer *) pos;
	    if (Timeval_diff(&t->timeout, &curr->timeout) < 0) {
		break;
	    }
	}
	List_add(pos->prev, &t->l);
    }

    return;
}

void Timer_set_timeout(struct timer *t, long msec)
{
    if (t->used) {
	Timer_remove(t);
    }

    gettimeofday(&t->timeout, NULL);

    if (msec < 0)
	printf("Negative timeout!!!\n");

    t->timeout.tv_usec += msec * 1000;  //nano-(micro)-ms-sec
    t->timeout.tv_sec += t->timeout.tv_usec / 1000000;
    t->timeout.tv_usec = t->timeout.tv_usec % 1000000;

    Timer_add(t);
}

int Timer_init(struct timer *t, timeout_func_t f, void *data)
{
	if (!t)
		return -1;
	
	INIT_LIST_ELM(&t->l);
	t->handler = f;
	t->data = data;
	t->timeout.tv_sec = 0;
	t->timeout.tv_usec = 0;
	t->used = 0;

    return 0;
}

void alm_start(void)
{
	gettimeofday(&this_host.fwd_time,NULL);

	if(sch_hello_check_timer.used){
		printf("Already sch_hello_check_timer is worked!\n");
	}
	else{
		Timer_init(&sch_hello_check_timer, (void *) &sch_hello_check, NULL);

		sch_hello_check();
	}

	if(alm_timer.used)
	{
		printf("Already alm_timer is worked!\n");
	}
	else{
		Timer_init(&alm_timer, (void *) &alm_calculator, NULL);

		alm_calculator();
	}

	if(RootNode==1){
		if(nms_alm_timer.used)
		{
			printf("Already nms_alm_timer is worked!\n");
		}
		else{
			Timer_init(&nms_alm_timer, (void *) &nms_alm_calculator, NULL);

			nms_alm_calculator();
		}
	}
}

void sch_hello_check(void)
{
	int i;
	
	for(i=0;i<10;i++){
		if (p_rt_list->rt_t[i].sch_hello_cnt>0){

			if(p_rt_list->rt_t[i].sch_hello_detector>0){
				p_rt_list->rt_t[i].alm_buffer[p_rt_list->rt_t[i].alm_buffer_pointer] = 1;
				p_rt_list->rt_t[i].alm_buffer_pointer++;
				if(p_rt_list->rt_t[i].alm_buffer_pointer==10 || p_rt_list->rt_t[i].alm_buffer_pointer>10){
					p_rt_list->rt_t[i].alm_buffer_pointer = 0;
				}
			}
			else{
				p_rt_list->rt_t[i].alm_buffer[p_rt_list->rt_t[i].alm_buffer_pointer] = 0;
				p_rt_list->rt_t[i].alm_buffer_pointer++;
				if(p_rt_list->rt_t[i].alm_buffer_pointer == 10 || p_rt_list->rt_t[i].alm_buffer_pointer > 10){
					p_rt_list->rt_t[i].alm_buffer_pointer = 0;
				}
			}
			p_rt_list->rt_t[i].sch_hello_detector = 0;
		}
	}
	Timer_set_timeout(&sch_hello_check_timer, ALM_INTERVAL);
}

void alm_calculator(void)
{
	int i,j,sum, diff;
	struct timeval now;
	gettimeofday(&now,NULL);
	
	for(i=0;i<10;i++){
		sum = 0;

		diff = now.tv_sec - p_rt_list->rt_t[i].first_hello_time.tv_sec;
		if (p_rt_list->rt_t[i].sch_hello_cnt>0 && p_rt_list->rt_t[i].hcnt==1)		{
			// printf("[!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!] : %u\n",p_rt_list->rt_t[i].alm);
			if(diff<10 && diff!=0){
				for(j=0;j<diff;j++){
					sum = sum + p_rt_list->rt_t[i].alm_buffer[j];
				}
				// printf("[!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!] : %d\n",sum);
				p_rt_list->rt_t[i].alm_next_hop = (sum*100) / diff;
			}
			else{
				for(j=0;j<10;j++){
					sum = sum + p_rt_list->rt_t[i].alm_buffer[j];
				}
				// printf("[!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!] : %d\n",sum);
				p_rt_list->rt_t[i].alm_next_hop = (sum*100) / 10;
			}
			p_rt_list->rt_t[i].alm = p_rt_list->rt_t[i].alm_next_hop;
			// printf("[!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!] : %u , %u , %u , %u , %u , %u , %u , %u , %u , %u\n",p_rt_list->rt_t[i].alm_buffer[0],p_rt_list->rt_t[i].alm_buffer[1],p_rt_list->rt_t[i].alm_buffer[2],p_rt_list->rt_t[i].alm_buffer[3],p_rt_list->rt_t[i].alm_buffer[4],p_rt_list->rt_t[i].alm_buffer[5],p_rt_list->rt_t[i].alm_buffer[6],p_rt_list->rt_t[i].alm_buffer[7],p_rt_list->rt_t[i].alm_buffer[8],p_rt_list->rt_t[i].alm_buffer[9]);
			// printf("[!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!] : %u\n",p_rt_list->rt_t[i].alm);
		}
	}
	Timer_set_timeout(&alm_timer, ALM_INTERVAL);
}

void nms_alm_calculator(void)
{
	int i,diff;
	ll_addr source,blank;
	struct timeval now;
	struct rt_table *rt;
	gettimeofday(&now,NULL);

	memset(&blank,0,6);
	
	for(i=0;i<10;i++){

		if(memcmp(&blank,p_nms_list->nms_t[i].addr,6) && memcmp(my_mac,p_nms_list->nms_t[i].addr,6)){
			
			diff = now.tv_sec - p_nms_list->nms_t[i].last_rann_ack_time.tv_sec;

			if (diff>10 && p_nms_list->nms_t[i].alm_next_hop!=0)
			{
				p_nms_list->nms_t[i].alm_next_hop = p_nms_list->nms_t[i].alm_next_hop/2;
				memcpy(&source,p_nms_list->nms_t[i].addr,6);
				rt = rt_table_find(source,0,source);
				if(rt){					
					rt->alm_next_hop = rt->alm_next_hop/2;
				}
			}
		}
	}
	
	Timer_set_timeout(&nms_alm_timer, NMS_ALM_INTERVAL);
}

void Data_Count()
{
	if((data_cnt == b_data_cnt) && (data_cnt > 0)) {
		data_cnt = 0;
		b_data_cnt = 0;
		printf("\n***** NO DATA for %d sec // RESET Data Count *****\n\n", DATA_INTERVAL/1000);

	}
	else b_data_cnt = data_cnt;

	Timer_set_timeout(&data_timer, DATA_INTERVAL);	

}



//NMS --------------------------------------------------------------------------------------------------------------------------------------------
// 1. NMS log add
// 2. Root면 UDP로 NMS에게 UDP 전송
// 3. 아니면 UpLink로 Log를 전달

//size = log size, msg = log 
void NMS_logging(unsigned int size, uint8_t* msg)
{	
	printf("\nIn NMS logging!!!\n");
	// printf("\n received rann size %u !!!\n", size);
	NMS_log_add(size, msg);
	
	if(RootNode == 1) {
		NMS_log_send_nms(p_nms_log_str,nms_log_buff,p_nms_log_str->size);		//
		NMS_log_reset();
	}
	else
		NMS_log_send();

	//printq(tq.rear);
}

void NMS_log_add(unsigned int size, uint8_t* msg)	
{
	printf("\nIn NMS_log_add!!!\n");
	memcpy(&nms_log_buff[p_nms_log_str->size],msg,size);
	p_nms_log_str->size += size;
}

//NMS log send along uplink
void NMS_log_send()
{
	printf("\nIn NMS_log_send!!!\n");
	uint8_t tbuff[MAX_FRAME_LEN]="";
	NMSLOGINFO *nmsstr;
	uint8_t* msgptr;

	struct rt_table *rt;

	ll_addr source;

	memcpy(&source,my_mac,6);

	// printf("\nIn 0!!!\n");

	rt = rt_table_find(root_addr,1,source);
	if(!rt){
		printf("Error : There is no route to Root\n");
	}
	else{
		// printf("\nIn 1!!!\n");
		nmsstr = (NMSLOGINFO *)tbuff;
		nmsstr->type = 6; //NMS DATA	
		nmsstr->size = p_nms_log_str->size;
		memcpy(nmsstr->orig_addr,my_mac,6);
		memcpy(dest_mac_addr, &rt->next_hop , ETH_ALEN); // DA

		// printf("\nIn 2!!!\n");

		msgptr = (uint8_t*)((char *)nmsstr + sizeof(NMSLOGINFO) );
		memcpy(msgptr, nms_log_buff, nmsstr->size);

		// printf("\nIn 3!!!\n");

		//print_d(tbuff, sizeof(DATA)+size);

		RANN* msg;	

		msg = (RANN *)((char *)msgptr);

		printf("nmsg_str->size : %u\n",nmsstr->size);
		printf("nmsg_str->type : %u\n",nmsstr->type);
		printf("nmsg_str->addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
		    nmsstr->orig_addr[0],nmsstr->orig_addr[1],nmsstr->orig_addr[2],
			nmsstr->orig_addr[3],nmsstr->orig_addr[4],nmsstr->orig_addr[5]);

		printf("msg->type = %u\n",msg->type);
		// printf("msg->addr= %02x:%02x:%02x:%02x:%02x:%02x\n",msg->orig_addr[0],msg->orig_addr[1],msg->orig_addr[2],msg->orig_addr[3],msg->orig_addr[4],msg->orig_addr[5]);
		// printf("msg->order = %u\n",msg->order);
			
		addq((char *)nmsstr->size + sizeof(NMSLOGINFO) , tbuff, 1,0);
		//printq(tq.rear);

		// printf("\nIn 4!!!\n");

		NMS_log_reset();
	}
}

void NMS_log_reset()
{
	// printf("\nIn NMS_log_reset!!!\n");
	memset(&nms_log_buff[0],0,MAX_FRAME_LEN-1);
	// printf("\nIn 5!!!\n");
	p_nms_log_str->size = 0;
	// printf("\nIn 6!!!\n");
}

// Root -> NMS (UDP send)    str = nms_log_str , msg = str제외한 log msg pointer, size = str제외한 log msg size 
void NMS_log_send_nms(NMSLOGINFO* str, uint8_t* msg, unsigned int size)
{
	// printf("\nIn NMS_log_send_nms!!!\n");

	static int outSock_udp_nms = 0;
	struct sockaddr_in addr_in;
	int bytes=0;
	uint8_t tbuff[MAX_FRAME_LEN]="";
	NMSLOGINFO *nmsstr;
	uint8_t* msgptr;

	if(NMS_Connection==0){
		printf("[In NMS_LOG send error] : There is no connection with NMS Client!\n");
	}
	else{
		memcpy(tbuff,str,sizeof(NMSLOGINFO));
		memcpy(&tbuff[sizeof(NMSLOGINFO)],msg,size);
		bytes = send(nms_tcp_send_client_ext,tbuff, size+sizeof(NMSLOGINFO), 0);

		if(bytes==-1)	
			printf("Send to NMS Error : %d\n",bytes);
		else
			printf("Log to NMS total send byte : %d\n",bytes);

	}
}

//buf = payload 맨 앞 (log_str+msg(aodv message)) , size = msg 의 길이 = log_str_size
void NMS_log_forward(uint8_t* buf, unsigned int size, ll_addr source)
{
	struct rt_table *rt;

	rt = rt_table_find(root_addr,1,source);
	if(!rt){
		printf("NMS_log_forward Error : There is no route to Root\n");
	}
	else{
		memcpy(dest_mac_addr, &rt->next_hop , ETH_ALEN); // DA
			
		addq(sizeof(NMSLOGINFO)+size , buf, 1,0);
		//printq(tq.rear);

		printf("\n[NMS log size %d] Send : Nms_log Forward _ To %02x:%02x:%02x:%02x:%02x:%02x\n",sizeof(NMSLOGINFO)+size, rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);
	}
	
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------


//Routing Protocol--------------------------------------------------------------------------------------------------------------------------------------------

void Hello_set(HELLO *hello, uint8_t hcnt, uint8_t *Src_addr, uint32_t ip4)
{
	hello->type = 0;	//AODV_HELLO

	hello->r_ch = receiving_channel;
	memcpy(hello->orig_addr,Src_addr,6);
	hello->orig_IP = ip4;
}

void Rann_set(RANN *rann, uint8_t *Src_addr)
{
	rann->type = 1;	//AODV_RANN
	memcpy(rann->orig_addr,Src_addr,6);
	rann->order = rann_order;
	rann->ttl = 3;
}

void Preq_set(PREQ *preq, uint8_t *Dst_addr, uint8_t *Src_addr, uint32_t order, uint32_t r_ch)
{
	preq->type = 2;	//AODV_PREQ

	memcpy(preq->dest_addr,Dst_addr,6);
	memcpy(preq->orig_addr,Src_addr,6);
	preq->hcnt = 1;
	preq->ttl = 4;
	preq->alm = 0;
	preq->seqno = order;
	memcpy(&preq->orig_IP,&my_ip,4);
	preq->r_ch = r_ch;
}

void Prep_set(PREP *prep, uint8_t hcnt, uint8_t *Dst_addr, uint8_t *Src_addr)
{
	prep->type = 3;	//AODV_PREP

	prep->hcnt = hcnt;
	memcpy(prep->dest_addr,Dst_addr,6);
	memcpy(prep->orig_addr,Src_addr,6);
	memcpy(&prep->orig_IP,&my_ip,4);  
}

void Perr_set(PERR *perr, uint8_t *timeout_addr, uint8_t *dest_addr)
{
	perr->type = 5;	//AODV_PERR
	memcpy(perr->timeout_addr,timeout_addr,6);
	memcpy(perr->dest_addr,dest_addr,6);
}


void Make_msg_format_HELLO(void)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	
	HELLO *hello = (HELLO *) tbuff;

	//Hello_set(hello, i, my_mac, my_ip);

	hello->type = 0;	//AODV_HELLO

	hello->r_ch = receiving_channel;
	memcpy(hello->orig_addr, my_mac,6);
	memcpy(&hello->orig_IP , &my_ip, 4);
	
	addq(sizeof(HELLO), tbuff, 0, 0);
	
}	

void Make_msg_format_SCH_HELLO(void)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	
	HELLO *hello = (HELLO *) tbuff;

	//Hello_set(hello, i, my_mac, my_ip);

	hello->type = 9;	//AODV_HELLO

	hello->r_ch = receiving_channel;
	memcpy(hello->orig_addr, my_mac,6);
	memcpy(&hello->orig_IP , &my_ip, 4);

	// printf("send sch hello\n");
	addq(sizeof(HELLO), tbuff, 0, 1);
}	

void Make_msg_format_RANN(ll_addr *reset_addr)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	RANN *rann =  (RANN *) tbuff;
	//Rann_set(rann, my_mac);

	rann->type = 1;	//AODV_RANN

	memcpy(rann->orig_addr, my_mac,6);
	rann->order = rann_order;
	rann->ttl = 3;		
	rann->metric = metric;
	rann->data_rate = data_rate;
	rann->reset = route_reset;
	rann->recv_channel = receiving_channel;
	rann->rann_id = rann_id;

	// 2 -> fast
	if(update_cycle == 0.5){
		rann->update_period = 2;
	}
	// 1 -> slow
	else if(update_cycle == 1.5){
		rann->update_period = 1;
	}
	// 0 -> default
	else{
		rann->update_period = 0;
	}

	if(nms_log_on==true)
		rann->log = 2;
	else
		rann->log = 1;

	memcpy(&rann->reset_addr, reset_addr, 6);
	route_reset = 0;

	addq(sizeof(RANN), tbuff, 0, 0);
}

void Make_msg_format_RANN_ACK(ll_addr *origin_addr, ll_addr *next_addr, uint32_t alm)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	RANN_ACK *rann_ack =  (RANN_ACK *) tbuff;

	rann_ack->type = 10;	//AODV_RANN

	memcpy(rann_ack->orig_addr, origin_addr,6);
	memcpy(rann_ack->next_addr, next_addr,6);
	rann_ack->alm_next_hop = alm;

	memcpy(dest_mac_addr, next_addr, ETH_ALEN); // DA

	addq(sizeof(RANN_ACK), tbuff, 1, 0);
}

void Make_msg_format_PREQ(ll_addr *route,  uint32_t order, uint32_t r_ch)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	PREQ *preq; 
	AODV_ext *ext;

	preq = (PREQ *) tbuff;
	//Preq_set(preq, (uint8_t *)route, my_mac, order);

	preq->type = 2;	//AODV_PREQ

	memcpy(preq->dest_addr,route,6);
	memcpy(preq->orig_addr,my_mac,6);
	preq->hcnt = 1;
	preq->ttl = 4;
	preq->alm = 0;
	preq->seqno = order;
	memcpy(&preq->orig_IP,&my_ip,4);
	preq->r_ch = receiving_channel;

	ext = (AODV_ext *) ((char *) tbuff + sizeof(PREQ) );
	memcpy(ext->addr,my_mac,6);
	
	printf("\nSend : Preq _ To root %02x:%02x:%02x:%02x:%02x:%02x\n",
	    route->addr[0],route->addr[1],route->addr[2],
	    route->addr[3],route->addr[4],route->addr[5]);

	addq(sizeof(PREQ)+sizeof(AODV_ext)*(preq->hcnt),tbuff, 0, 0);
	//printq(tq.rear);
}

void Make_msg_format_PREP(PREQ * preq)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	PREP *prep;
	AODV_ext *preq_ext, *prep_ext;	

	prep = (PREP *) tbuff;
	memcpy(dest_mac_addr, preq->orig_addr, ETH_ALEN); // DA

	//Prep_set(prep, hcnt, (uint8_t *)preq->orig_addr, root_addr.addr);

	prep->type = 3;	//AODV_PREP
	prep->seqno = preq->seqno;
	prep->hcnt = 1;
	memcpy(prep->dest_addr,preq->orig_addr,6);
	memcpy(prep->orig_addr,root_addr.addr,6);
	memcpy(&prep->orig_IP,&my_ip,4);  
	memcpy(&prep->dest_IP,&preq->orig_IP,sizeof(uint32_t));

	preq_ext = (AODV_ext *)((char *)preq+sizeof(PREQ));
	prep_ext = (AODV_ext *)((char *)prep+sizeof(PREP));

	prep->ext_length = preq->hcnt-1;
	//printf("prep's ext_length is %d\n",prep->ext_length+1);

	
	if(prep->ext_length>0)
	{
		memcpy(prep_ext,preq_ext,sizeof(AODV_ext)*(prep->ext_length));
		preq_ext = (AODV_ext *)((char *)preq_ext+sizeof(AODV_ext)*(prep->ext_length));
		memcpy(dest_mac_addr, preq_ext->addr, ETH_ALEN); // DA -NextHop

		printf("\nSend : Prep _ To next %02x:%02x:%02x:%02x:%02x:%02x\n",dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);

		
	}
	else if(prep->ext_length==0)
	{
		memcpy(dest_mac_addr, preq_ext->addr, ETH_ALEN); // DA -NextHop
		printf("\nSend : Prep _ To %02x:%02x:%02x:%02x:%02x:%02x\n",dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);

	}
	else
	{
		printf("ext_length Error!!\n");
	}


	addq(sizeof(PREP)+sizeof(AODV_ext)*(prep->ext_length),tbuff, 1, 0);
	// printq(tq.rear);
}

void Make_msg_format_DATA(ll_addr *route,unsigned int size, char * msg)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	DATA *data;
	char *msgptr;

	data = (DATA *)tbuff;
	memcpy(dest_mac_addr, route, ETH_ALEN); // DA
	data->type = 4; //AODV DATA
	data->size = size;

	msgptr = (char *)((char *)data + sizeof(DATA) );
	memcpy(msgptr, msg, size);

	//print_d(tbuff, sizeof(DATA)+size);
		
	addq(sizeof(DATA)+size , tbuff, 1,1);
	//printq(tq.rear);
}

void Make_msg_format_PERR(ll_addr *timeout_addr,ll_addr *dest_addr)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	PERR *perr;
	
	perr = (PERR *)tbuff;
	//Perr_set(perr,(uint8_t *)timeout_addr,(uint8_t *)dest_addr);

	perr->type = 5;	//AODV_PERR
	memcpy(perr->timeout_addr,timeout_addr,6);
	memcpy(perr->dest_addr,dest_addr,6);
	addq(sizeof(PERR), tbuff, 0, 0);

	if(nms_log_on==true)
		NMS_logging(sizeof(PERR),tbuff);

	// printq(tq.rear);
}

void Make_msg_format_PERR_unicast(ll_addr *timeout_node)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	PERR_UNI *perr;
	struct rt_table *rt;
	ll_addr source;
	
	perr = (PERR_UNI *)tbuff;
	//Perr_set(perr,(uint8_t *)timeout_addr,(uint8_t *)dest_addr);

	memcpy(&source,my_mac,6);

	rt = rt_table_find(root_addr,1,source);
	if(!rt){
		printf("PERR_UNICAST Error : There is no route to Root\n");
	}


	memcpy(dest_mac_addr, &rt->next_hop, ETH_ALEN); // DA

	perr->type = 7;	//AODV_PERR_unicast
	memcpy(perr->timeout_addr,timeout_node,6);
	memcpy(perr->orig_addr,&source,6);
	addq(sizeof(PERR_UNI), tbuff, 0, 0);

	// printq(tq.rear);

}

void Hello_send(void)
{	
	Make_msg_format_HELLO();

	timer_cnt ++; 

	ll_addr source,dest;
	struct nms_table *nms;

	if(RootNode==1){
		memcpy(&source,my_mac,6);
		memset(&dest,0,6);
		nms = nms_table_find(source);
		if(!nms) {
			nms_table_insert(source, dest, receiving_channel, 1,0);
		}
	}

	if(timer_cnt <= config_time*100) {
		Timer_set_timeout(&hello_timer, HELLO_INTERVAL);
		command_on = true;
	}
	else {
		printf("\n-----------------  > CONFIG END  < ---------------\n");
		printf("\n-----------------  > CONFIG END  < ---------------\n");
		if(RootNode == 1)
			nms_table_print();
		rt_table_print();
		Timer_remove(&hello_timer);
		Timer_remove(&sch_hello_timer);
		Timer_remove(&sch_hello_check_timer);
		Timer_remove(&rann_timer);
		Timer_remove(&alm_timer);
		Timer_remove(&hello_timeout_timer);
		Timer_remove(&data_timer);

		config_end = true;

		Timer_init(&data_timer, (void *)&Data_Count, NULL);
		Data_Count(NULL);
	}

}

void SCH_Hello_send(void)
{	
	Make_msg_format_SCH_HELLO();
	// printq(tq.rear);

	timer_cnt ++; //1sec * 45 = 1min 30

	ll_addr source,dest;
	struct nms_table *nms;
	// printf("sch_hello_send!!!\n");	

	if(RootNode==1){
		memcpy(&source,my_mac,6);
		memset(&dest,0,6);
		nms = nms_table_find(source);
		if(!nms) {
			// printf("root insert!!!\n");
			nms_table_insert(source, dest, receiving_channel, 1,0);
		}
	}

	if(timer_cnt <= config_time*2000) {
		// if (config_time >= 30 && timer_cnt % 21 == 0)
		// 	rt_table_print();
		// else if ( config_time < 30 && timer_cnt % 11 == 0)
		// 	rt_table_print();

		Timer_set_timeout(&sch_hello_timer, SCH_HELLO_INTERVAL);
		// printf("now : %d")
		command_on = true;
	}
	else {
		printf("\n-----------------  > CONFIG END  < ---------------\n");
	}
}

void Rann_send()
{	
	// printf("rann..\n");
	ll_addr blank;

	memset(&blank, 0, 6);

	rann_order++;
	Make_msg_format_RANN(&blank);
	Timer_set_timeout(&rann_timer, (long)RANN_INTERVAL*update_cycle);

	//printq(tq.rear);
}

void Rann_ack_send()
{	
	ll_addr source, next_addr;
	struct rt_table *rt,*next_hop;

	memcpy(&source,my_mac,6);

	rt = rt_table_find(root_addr,1,source);
	if (!rt)
		printf("There is no route to root_addr\n");
	else
	{
		memcpy(&next_addr,&rt->next_hop,6);
		next_hop = rt_table_find(next_addr,0,source);
		if(!next_hop)
			printf("There is no next_hop to root in routing table\n");
		else{
			Make_msg_format_RANN_ACK(&source, &next_addr, 0);
		}
	}	
}


void Preq_send(ll_addr *route, uint32_t order,uint32_t r_ch)
{	
	Make_msg_format_PREQ(route, order,r_ch);
	//printq(tq.rear);        
}

void Preq_forward(PREQ *p_pre)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	PREQ * preq;
	AODV_ext *ext, *e_pre;

	preq = (PREQ *) tbuff;
	memcpy(preq,p_pre,sizeof(PREQ));

	preq->ttl = p_pre->ttl - 1;

	ext = (AODV_ext *) ((char *) tbuff + sizeof(PREQ) );
	e_pre = (AODV_ext *)((char*) p_pre + sizeof(PREQ) );

	if(preq->ttl<=0)
		printf("This msg is dropped : TTL is over\n");
	else
	{
		memcpy(preq->orig_addr,p_pre->orig_addr,6);
		memcpy(&preq->orig_IP,&p_pre->orig_IP,4);

		printf("\nSend : Preq forward _ broadcast\n");

		memcpy(ext,e_pre,sizeof(AODV_ext)*(p_pre->hcnt));
		ext = (AODV_ext *) ((char*) ext + sizeof(AODV_ext)*(p_pre->hcnt));		
		memcpy(ext->addr,my_mac,6);

		preq->hcnt = p_pre->hcnt + 1;

		addq(sizeof(PREQ)+sizeof(AODV_ext)*(preq->hcnt),tbuff, 0, 0);
		// printq(tq.rear);

	}

}


void Prep_send(PREQ *preq)
{
	ll_addr dest;
	int index;
	struct nms_table *nms;
	AODV_ext *ext;

	ext = (AODV_ext *)((char *)preq+sizeof(PREQ));

	memcpy(dest.addr,&preq->orig_addr[0],sizeof(ll_addr));

	index = rt_table_get_index(dest);
	if(index >= 0) tmp_preq[index].used = false;

	Make_msg_format_PREP(preq);
	
	if(RootNode == 1){
		nms = nms_table_find(dest);
		if(!nms){
			nms_table_insert(dest, dest, receiving_channel, 1,0);
		}
		else
			nms_table_uplink_insert(nms, ext, preq->hcnt);
	}
}


void Prep_forward(PREP *pre)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	PREP* prep;
	AODV_ext *pre_ext, *ext;

	prep = (PREP *) tbuff;
	ext =  (AODV_ext *)((char *)tbuff+sizeof(PREP));

	memcpy(prep,pre,sizeof(PREP));

	pre_ext = (AODV_ext *)((char *)pre+sizeof(PREP));

	prep->ext_length = pre->ext_length-1; //0
	prep->orig_IP = pre->orig_IP;
	prep->dest_IP = pre->dest_IP;
 
	if(pre_ext->addr[0]==0 && pre_ext->addr[1]==0 && pre_ext->addr[2]==0 && pre_ext->addr[3]==0 && pre_ext->addr[4]==0 && pre_ext->addr[5]==0)
		printf("This msg is dropped : Prep dest is 0.0.0.0.0.0\n");
	else
	{
	
		if(prep->ext_length>0)
		{
			memcpy(ext,pre_ext,sizeof(AODV_ext)*(prep->ext_length));
       			pre_ext = (AODV_ext *)((char *)pre_ext+sizeof(AODV_ext)*(prep->ext_length));
  			
			memcpy(dest_mac_addr, pre_ext->addr, ETH_ALEN); //last ext info = next hop unicast.

			printf("\nSend : Prep _ forward _ To next %02x:%02x:%02x:%02x:%02x:%02x\n",
			    dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],
				dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);

		}
		else if(prep->ext_length==0)
		{
			memcpy(dest_mac_addr, pre_ext->addr, ETH_ALEN);

			printf("\nSend : Prep _ forward _ To %02x:%02x:%02x:%02x:%02x:%02x\n",
		        dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2]
				dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);
			
		}
		else
		{
			printf("ext_length Error!!\n");
		}

		
		addq(sizeof(PREP)+sizeof(AODV_ext)*(prep->ext_length),tbuff, 1, 0);
		//printq(tq.rear);
	}
}


void Perr_send(ll_addr *timeout, ll_addr *dest)
{
	Make_msg_format_PERR(timeout,dest);

	printf("\nSend : Perr _ To %02x:%02x:%02x:%02x:%02x:%02x\n",
	    dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],
		dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);	
	
}

void Perr_unicast(ll_addr *timeout_node)
{
	Make_msg_format_PERR_unicast(timeout_node);

	memset(dest_mac_addr, &root_addr, 6);

	printf("\nSend : Perr unicast _ To root %02x:%02x:%02x:%02x:%02x:%02x\n",dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);	
}

void Perr_forward(PERR_UNI *per)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";
	PERR_UNI* perr;
	ll_addr source;
	struct rt_table *rt;

	perr = (PERR_UNI *) tbuff;

	memcpy(perr,per,sizeof(PERR_UNI));
	memcpy(&source,perr->orig_addr,6);

	rt= rt_table_find(root_addr,1,source);
	if (!rt)
		printf(" Uplink was broken!!!\n");
	else
	{
		memcpy(dest_mac_addr, &rt->next_hop, ETH_ALEN); // DA
		addq(sizeof(PERR_UNI),tbuff, 1, 0);
		//printq(tq.rear);
	}	
	
}

void Hello_start(void)
{
	config_end = false;
	timer_cnt = 0; 

	Timer_remove(&data_timer);
	data_cnt = 0;
	b_data_cnt = 0;

	if(hello_timer.used)
	{
		printf("Already hello_timer is worked!\n");
	}
	else{

		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&hello_timer,(void *) &Hello_send, NULL);

		timer_cnt = 0;
		Hello_send();
	}
}

void SCH_Hello_start(void)
{
	config_end = false;
	timer_cnt = 0; 

	Timer_remove(&data_timer);
	data_cnt = 0;
	b_data_cnt = 0;

	if(sch_hello_timer.used)
	{
		printf("Already sch_hello_timer is worked!\n");
	}
	else{

		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&sch_hello_timer,(void *) &SCH_Hello_send, NULL);

		timer_cnt = 0;
		SCH_Hello_send();
	}
}

void Rann_forward(RANN *pre)
{	
	//uint8_t tbuff[MAX_FRAME_LEN]="";

	if(!config_end)	{
	printf("\nSend : Rann forward _ To %02x:%02x:%02x:%02x:%02x:%02x\n\n",
	    dest_mac_addr[0],dest_mac_addr[1],dest_mac_addr[2],
	    dest_mac_addr[3],dest_mac_addr[4],dest_mac_addr[5]);	
	}
	
	pre->ttl = pre->ttl - 1;
	addq(sizeof(RANN), (uint8_t*)pre, 0, 0);	
	//printq(tq.rear);
}

void Rann_start(void)
{
	// printf("In rann start\n");
	srand(getpid());
	rann_id = rand()%256;
	printf("rann_id = %d\n",rann_id);
	memcpy(&root_addr,my_mac,6);
	if(rann_timer.used)
	{
		printf("Already rann_timer is worked!\n");
	}
	else{
		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&rann_timer, (void *) &Rann_send, NULL);

		Rann_send();
	}
}


void Prep_start(int index)
{
	if(route_select_timer[index].used)
	{
		printf("Already prep_timer is worked!\n");
	}
	else{
		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&route_select_timer[index], (void*) &Prep_send, &tmp_preq[index].t_preq);

	        // Prep_send(&tmp_preq[index].t_preq); //send twice...

		// Timer_set_timeout(&route_select_timer[index], RANN_INTERVAL/10*3);
		Timer_set_timeout(&route_select_timer[index], RANN_INTERVAL/5*3);
	}
}

void Data_send_dst(char *msg,unsigned int size)
{
	static int outSock_udp = 0,outSock_tcp = 0;

	if(outSock_udp==0&&outSock_tcp==0)
	{
		outSock_udp = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
		outSock_tcp = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	}

	struct sockaddr_in addr_in;

	struct iphdr *ip = (struct iphdr *) msg;
//	struct udphdr *udp = (struct udphdr *) (msg + sizeof(struct iphdr));

	//int dstPort = 8888;	

	int bytes=0;
	
	int one = 1;
  	const int *val = &one;

	if (outSock_udp < 0)
		printf("*** Tx_openSocket failed!_UDP ***\n");
	if (outSock_tcp < 0)
		printf("***  Tx_openSocket failed!_TCP***\n");
	
	if(setsockopt(outSock_udp, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
	    	perror("setsockopt() error_UDP");
	if(setsockopt(outSock_tcp, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
	    	perror("setsockopt() error_TCP");
		dat
	addr_in.sin_family = AF_INET;
  	addr_in.sin_port = htons(PORT);
  	//addr_in.sin_addr.s_addr = inet_addr(pIPH->SrcIP);	
	addr_in.sin_addr.s_addr = my_ip;	

      //  printf("Protocol %d (17:UDP, 6:TCP)\n", ip->protocol);
	if(ip->protocol == 17)		//UDP
	{
		//ip->check = csum((unsigned short *)msg,sizeof(struct iphdr)+sizeof(struct udphdr));
		bytes = sendto(outSock_udp, msg, size, 0, (struct sockaddr *) &addr_in, sizeof(addr_in));
	}
	
	else if(ip->protocol == 6)	//TCP
	{	
		//ip->check = csum((unsigned short *)msg,sizeof(struct iphdr)+sizeof(struct tcphdr));
		bytes = sendto(outSock_tcp, msg, size, 0, (struct sockaddr *) &addr_in, sizeof(addr_in));
	}
	data_cnt++; //add neuron

	if(bytes==-1)	
		printf("Sendto Error : %d\n",bytes);
	else
		printf("\n[Send %d] : Data _ To %d.%d.%d.%d\n",data_cnt, my_ip&0xFF, 
		(my_ip>>8)&0xFF, (my_ip>>16)&0xFF,(my_ip>>24)&0xFF);
	
}

void Data_forward(char *buf,uint32_t DstIP, unsigned int size)
{
	unsigned char *p1 = (unsigned char*)&DstIP;
	struct rt_table *rt;

	rt = rt_table_find_IP(DstIP);

	if (!rt) {
		printf("Error : [Dst] %d.%d.%d.%d is unknown IP.\n", p1[0],p1[1],p1[2],p1[3]);
		//PERR is needed
	}
	else {

		Make_msg_format_DATA(&rt->next_hop, size, buf);
		//printq(tq.rear);
		
		data_cnt++; //add neuron
		printf("\n[DATA %d] Send : Data Forward _ To %02x:%02x:%02x:%02x:%02x:%02x\n",
		    data_cnt, rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],
		    rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);
	
	}
	
}

void Data_start(char* msg, unsigned int size)
{
	struct rt_table *rt;
	//print_d(msg, (int) size);

	char pMsg[MAX_FRAME_LEN]="";
	memcpy(pMsg,msg, size);
	
	rt = rt_table_find_IP(pIPH->DstIP);
	
	if (!rt) {
		printf("Error 11: [Dst] %i.%i.%i.%i is unknown IP.\n", 
		    (pIPH->DstIP) & 0xFF, ((pIPH->DstIP) >> 8) & 0xFF, 
			((pIPH->DstIP) >> 16) & 0xFF, ((pIPH->DstIP)>>24) & 0xFF);
	} 
	else {

		Make_msg_format_DATA(&rt->next_hop, size, pMsg);	
			
		printf("\n[DATA] Send : DATA _ To %02x:%02x:%02x:%02x:%02x:%02x\n",
		    rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],
			rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);
		
	}

}

void Data_start2(ll_addr *route, char* msg, int size)
{
	char tbuff[MAX_FRAME_LEN]="";

	int i = 0; 
	DATA *data;
	char *msgptr;

	data = (DATA *)tbuff;
	memcpy(dest_mac_addr, route->addr, ETH_ALEN); // DA
	msgptr = (char*) ((char*)tbuff+ sizeof(DATA));
	memcpy(msgptr, msg, size);

	data->type = 8; //AODV DATA
	data->size = size;

	addq(sizeof(DATA)+size , tbuff, 1,1);
}


//test

void Data_start3(ll_addr *route, char* msg, int size)
{
	char tbuff[MAX_FRAME_LEN]="";

	int i = 0; 
	DATA *data;
	char *msgptr;

	data = (DATA *)tbuff;
	memcpy(dest_mac_addr, route->addr, ETH_ALEN); // DA
	msgptr = (char*) ((char*)tbuff+ sizeof(DATA));
	memcpy(msgptr, msg, size);

	data->type = 8; //AODV DATA
	data->size = size;

	addq(sizeof(DATA)+size , tbuff, 1,0);
}

void PREP_TEST_CCH(ll_addr *route)
{
	char tbuff[MAX_FRAME_LEN]="";

	int i = 0; 
	PREP *prep;
	char *msgptr;

	prep = (PREP *)tbuff;
	memcpy(dest_mac_addr, route->addr, ETH_ALEN); // DA
	msgptr = (char*) ((char*)tbuff+ sizeof(DATA));
	memcpy(msgptr, prep, sizeof(PREP));

	prep->type = 3;	//AODV_PREP
	prep->seqno = 1;
	prep->hcnt = 1;
	memcpy(prep->dest_addr,route,6);
	memcpy(prep->orig_addr,my_mac,6);
	memcpy(&prep->orig_IP,&my_ip,4);  
	memcpy(&prep->dest_IP,&my_ip,sizeof(uint32_t));

	addq(sizeof(PREP) , tbuff, 1,0);
}

void PREP_TEST_SCH(ll_addr *route)
{
	char tbuff[MAX_FRAME_LEN]="";

	int i = 0; 
	PREP *prep;
	char *msgptr;

	prep = (PREP *)tbuff;
	memcpy(dest_mac_addr, route->addr, ETH_ALEN); // DA
	msgptr = (char*) ((char*)tbuff+ sizeof(DATA));
	memcpy(msgptr, prep, sizeof(PREP));

	prep->type = 3;	//AODV_PREP
	prep->seqno = 1;
	prep->hcnt = 1;
	memcpy(prep->dest_addr,route,6);
	memcpy(prep->orig_addr,my_mac,6);
	memcpy(&prep->orig_IP,&my_ip,4);  
	memcpy(&prep->dest_IP,&my_ip,sizeof(uint32_t));

	addq(sizeof(PREP) , tbuff, 1,1);
}


void Hello_timeout_process(void)
{
	printf("In timeout process\n");
	struct timeval now;
	struct rt_table *rt;
	int diff,i,j;
	ll_addr source,timeout_node;
	bool sent = false;

	gettimeofday(&now,NULL);


	timer_cnt++;
	
	for(i=0;i<10;i++)
	{
		sent = false;
		if (p_rt_list->rt_t[i].hello_cnt>0)
		{

			diff = now.tv_sec - p_rt_list->rt_t[i].last_hello_time.tv_sec;
			printf("diff = %d\n",diff);
			
			//NMS Update주기를 Hello timeout에 반영시킨 것. 0.5배 -> 5초가 너무 짧을 수 있음. 통신환경에 따라 달리 적용 필요
			if(diff > (int)(10*update_cycle))
			{

				printf("Hello Timeout : %02x:%02x:%02x:%02x:%02x:%02x neighbor is timeout. This neighbor is deleted\n",
				    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
					p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);

				memcpy(&source,my_mac,6);
				memcpy(&timeout_node,&p_rt_list->rt_t[i].dest_addr,6);

				Perr_send(&timeout_node,&timeout_node);

				//Next hop이 죽은 노드인 routing table 삭제 
				for(j=0;j<10;j++)
				{
					if(j==i)
						continue;
					if(!memcmp(&p_rt_list->rt_t[j].next_hop,&timeout_node,6))
					{
						printf("Hello Timeout : To %02x:%02x:%02x:%02x:%02x:%02x route is deleted. Next hop is deleted\n",
						    p_rt_list->rt_t[j].dest_addr.addr[0],p_rt_list->rt_t[j].dest_addr.addr[1],p_rt_list->rt_t[j].dest_addr.addr[2],
						    p_rt_list->rt_t[j].dest_addr.addr[3],p_rt_list->rt_t[j].dest_addr.addr[4],p_rt_list->rt_t[j].dest_addr.addr[5]);

						if(!memcmp(&p_rt_list->rt_t[j].dest_addr,&root_addr,6)){
							if(sent==false){
								Perr_send(&timeout_node,&source);
								sent = true;
							}
						}
						#if 0
						memset(&p_rt_list->rt_t[j], 0, sizeof(struct rt_table));
						p_rt_list->rt_t[j].ifindex = if_nametoindex("wave-data0");
						p_rt_list->rt_t[j].hcnt = 0;
						p_rt_list->rt_t[j].hello_cnt = 0;
						p_rt_list->rt_t[j].state = 0;	//empty	
						#endif
						rt_table_empty(j);
						continue;
					}
				}

				//Source가 죽은 노드인 routing table 삭제 
				for(j=0;j<10;j++)
				{
					if(j==i)
						continue;
					if(!memcmp(&p_rt_list->rt_t[j].source_addr,&timeout_node,6))
					{
						printf("Hello Timeout : To %02x:%02x:%02x:%02x:%02x:%02x route is deleted. source is deleted\n",
						    p_rt_list->rt_t[j].dest_addr.addr[0],p_rt_list->rt_t[j].dest_addr.addr[1],p_rt_list->rt_t[j].dest_addr.addr[2],
							p_rt_list->rt_t[j].dest_addr.addr[3],p_rt_list->rt_t[j].dest_addr.addr[4],p_rt_list->rt_t[j].dest_addr.addr[5]);

						#if 0
						memset(&p_rt_list->rt_t[j], 0, sizeof(struct rt_table));
						p_rt_list->rt_t[j].ifindex = if_nametoindex("wave-data0");
						p_rt_list->rt_t[j].hcnt = 0;
						p_rt_list->rt_t[j].hello_cnt = 0;
						p_rt_list->rt_t[j].state = 0;	//empty	
						#endif
						rt_table_empty(j);
						continue;
					}
				}

				for(j=0;j<10;j++)
				{
					if(j==i)
						continue;
					if(!memcmp(&p_rt_list->rt_t[j].prev_addr,&timeout_node,6))
					{
						printf("Hello Timeout : To %02x:%02x:%02x:%02x:%02x:%02x route is deleted. Prev hop is deleted\n",
						    p_rt_list->rt_t[j].dest_addr.addr[0],p_rt_list->rt_t[j].dest_addr.addr[1],p_rt_list->rt_t[j].dest_addr.addr[2],
							p_rt_list->rt_t[j].dest_addr.addr[3],p_rt_list->rt_t[j].dest_addr.addr[4],p_rt_list->rt_t[j].dest_addr.addr[5]);

						#if 0
						memset(&p_rt_list->rt_t[j], 0, sizeof(struct rt_table));
						p_rt_list->rt_t[j].ifindex = if_nametoindex("wave-data0");
						p_rt_list->rt_t[j].hcnt = 0;
						p_rt_list->rt_t[j].hello_cnt = 0;
						p_rt_list->rt_t[j].state = 0;	//empty	
						#endif
						rt_table_empty(j);
						continue;
					}
				}

				#if 0
				memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
				p_rt_list->rt_t[i].ifindex = 0; //default, if_nametoindex("wave-raw");
				p_rt_list->rt_t[i].hcnt = 0;
				p_rt_list->rt_t[i].hello_cnt = 0;
				p_rt_list->rt_t[i].state = 0;	//empty	
				#endif
				rt_table_empty(i);

				if(RootNode == 1){
				nms_table_perr_update(timeout_node,timeout_node);
				}
				else{
					rt= rt_table_find(root_addr,1,source);rann
					if (!rt)
						printf(" Uplink was broken!!!\n");
					else
						Perr_unicast(&timeout_node);
				}

			}
		}
	}

	// rt_table_print();

	if(timer_cnt < 1000)
		Timer_set_timeout(&hello_timeout_timer, HELLO_TIMEOUT_INTERVAL);
	else timer_cnt = 0;
}


void Hello_timeout_start(void)
{
	printf("In hello_timeout_setup\n");

	if(hello_timeout_timer.used)
	{
		printf("Already hello_timeout_timer is worked!\n");
	}
	else
	{
		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&hello_timeout_timer,(void *) &Hello_timeout_process, NULL);

		Hello_timeout_process();
	}
}


void Hello_process(uint8_t * pBuf)
{
	HELLO *hello;
	ll_addr dest, source;
	struct timeval now;
	struct rt_table *rt=NULL;
	struct nms_table *nms;

	hello = (HELLO * ) pBuf;

	if(!config_end){
		printf("Recv : Hello _ From %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac_addr[0],
		src_mac_addr[1],src_mac_addr[2],src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);
	}
	gettimeofday(&now, NULL);

	memcpy(&dest,hello->orig_addr,6);
	memcpy(&source,my_mac,6);
	
	//Don't care about soure 
    rt = rt_table_find(dest,0,dest);

	if (!rt) {
 		if(hello->orig_IP !=0){
			rt_table_insert(dest, dest, source, 0, source, 1 , hello->orig_IP, 1,&now,0.0,0);
		}
	} 
	else {
		rt->hello_cnt++;
		memcpy(&rt->last_hello_time,&now,sizeof(struct timeval));
		if(rt->dest_IP == 0 && hello->orig_IP !=0)
			rt->dest_IP =hello->orig_IP;
	}

	if(RootNode == 1){
		//Root의 NMS 관리
		nms = nms_table_find(source);
		if(!nms) {
			printf("root insert!!!\n");
			nms_table_insert(source, dest, hello->r_ch, 1,0);
		}
		else{
			// printf("root update!!!\n");
			nms_table_update(nms, dest, hello->r_ch, 1,0);
		}

		//Hello 보낸 node의 NMS 관리
		nms = nms_table_find(dest);
		if(!nms) {
			printf("neighbor insert!!!\n");
			nms_table_insert(dest, source, receiving_channel, 1,1);
		}
		else{
			// printf("neighbor update!!!\n");
			nms_table_update(nms, source, receiving_channel, 1,1);
		}
	}
}

void SCH_Hello_process(uint8_t * pBuf)
{
	HELLO *hello;
	ll_addr dest, source;
	struct timeval now;
	struct rt_table *rt=NULL;
	struct nms_table *nms;

	hello = (HELLO * ) pBuf;

	memcpy(&dest,hello->orig_addr,6);

	rt = rt_table_find(dest,0,dest);

	if (rt) {
		// if(!config_end){
			// printf("Recv : SCH_Hello _ From %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);
		// }
		gettimeofday(&now, NULL);

		rt->sch_hello_cnt++;
		rt->sch_hello_detector++;

		memcpy(&rt->last_hello_time,&now,sizeof(struct timeval));
	} 
}


void Rann_process(uint8_t * pBuf)
{
	RANN *rann;
	
	struct timeval now;
	struct rt_table *rt;

	ll_addr source;

	rann = (RANN *) pBuf;

	
	if(!config_end) 
		printf("Recv : Rann _ From %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);

	gettimeofday(&now, NULL);

	if(!config_end){
		printf("Rann_order = %d / comp = %d \n",rann->order,rann_order);
		printf("Rann_id = %d / comp = %d \n",rann->rann_id,rann_id);
	}

	if(rann->rann_id!=rann_id){
		rann_id = rann->rann_id;
		rann_order = 0;
		rt_NMS_reset();
	}

	if(rann->order>rann_order)
	{
		if(RootNode!=1){
			rann_order = rann->order;
			memcpy(&root_addr,rann->orig_addr,6);
			memcpy(&source,my_mac,6);

			if(nms_log_on==true)
				NMS_logging(sizeof(RANN),pBuf);

			if(rann->ttl-1>0)
				Rann_forward(rann);
			
			//metric 변경 처리
			if(rann->metric!=metric){
				if(rann->metric!=0){
					printf("[In Rann_process] : metric is changed : %d -> %d  by NMS-control\n",metric, rann->metric);	
					metric = rann->metric;
					// api_reset = 1;
					// Timer_set_timeout(&API_reset_timer, API_RESET_INTERVAL);
					rt_NMS_reset();
				}
			}

			//recv_channel 변경 처리
			if(rann->recv_channel!=receiving_channel){
				if(rann->recv_channel!=0){
					printf("[In Rann_process] : recv_channel is changed : %d -> %d by NMS-control\n",receiving_channel, rann->recv_channel);	
					receiving_channel = rann->recv_channel;
					api_reset = 1;
					Timer_set_timeout(&API_reset_timer, API_RESET_INTERVAL);
					rt_NMS_reset();
				}
			}

			//data_rate 변경 처리
			if(rann->data_rate!=data_rate){
				if(rann->data_rate!=0){
					printf("[In Rann_process] : datarate is changed : %d -> %d by NMS-control\n",data_rate, rann->data_rate);	
					data_rate = rann->data_rate;
					api_reset = 1;
					Timer_set_timeout(&API_reset_timer, API_RESET_INTERVAL);
					rt_NMS_reset();
				}
			}

			//Reset 명령 처리
			if(rann->reset==1){
				printf("[In Rann_process] : rt_NMS reset by NMS-control\n");	
				rt_NMS_reset();
			}

			//Log 변경 처리
			if(rann->log==2){
				if(nms_log_on == false){
					printf("[In Rann_process] : log on by NMS-control\n");	
					nms_log_on = true;
				}
			}else{
				if(nms_log_on == true){
					printf("[In Rann_process] : log off by NMS-control\n");	
					nms_log_on = false;
				}
			}

			//update period 처리. 0 -> default / 1 -> slow / 2 -> fast
			if(rann->update_period == 2){
				if(update_cycle != 0.5){
					update_cycle == 0.5;
					printf("[In Rann_process] : update period is changed to %f\n",update_cycle);	
				}
			}
			else if(rann->update_period ==1){
				if(update_cycle != 1.5){
					update_cycle == 1.5;
					printf("[In Rann_process] : update period is changed to %f\n",update_cycle);	
				}
			}
			else{
				if(update_cycle != 1){
					update_cycle == 1;
					printf("[In Rann_process] : update period is changed to %f\n",update_cycle);	
				}
			}

			//Reset_addr 처리
			if(memcmp(my_mac,&rann->reset_addr,6)==0){
				printf("[In Rann_process] : reboot by NMS-control\n");	
				reboot_flag = 1;
				Timer_set_timeout(&API_reset_timer, API_RESET_INTERVAL);
			}

			rt = rt_table_find(root_addr,1,source);
			if (!rt) {			
				Preq_send(&root_addr,rann_order,receiving_channel);
			} 
			else{
				Rann_ack_send();
			}
		}
	}
	else
	{
		if(!config_end) 
			printf("RANN msg is dropped : Repeated Msg  \n");
	}
}


void Rann_ack_process(uint8_t * pBuf)
{
	RANN_ACK *rann_ack;
	
	struct timeval now;
	struct rt_table *rt;
	struct nms_table *nms;

	ll_addr source, next, neighbor, my;

	rann_ack = (RANN_ACK *) pBuf;
	memcpy(&source,rann_ack->orig_addr,6);

	gettimeofday(&now, NULL);

	if(!config_end) 
		printf("[In Rann_ack_process] Recv : Rann_ack _ From %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);

	rt = rt_table_find(source,0,source);
	if(rt != NULL){
		if(memcmp(my_mac,rann_ack->next_addr,6)==0 && rt->hcnt == 1){
			printf("	The Rann_ack orig is neighbor. rann_ack->alm = %u / rt-> alm = %u /  =>> rann_ack->alm => %u\n",rann_ack->alm_next_hop,rt->alm_next_hop,rann_ack->alm_next_hop + rt->alm_next_hop);
			rann_ack->alm_next_hop = rt->alm_next_hop;
		}
		else if(memcmp(my_mac,rann_ack->next_addr,6)!=0){
			printf("	Error : Forwarding error\n");
			return NULL;
		}
	}
	else{
		printf("	Error : Routing table error. But There is no route information about %02x:%02x:%02x:%02x:%02x:%02x\n",rann_ack->orig_addr[0],rann_ack->orig_addr[1],rann_ack->orig_addr[2],rann_ack->orig_addr[3],rann_ack->orig_addr[4],rann_ack->orig_addr[5]);
		return NULL;
	}
		
	if(RootNode!=1){
		rt = rt_table_find(root_addr,1,source);
		if (!rt) {
			printf("	Error : Rann_ack receive. But there is no route to root\n");
			return NULL;
		} 
		else{
			printf("	Forwarding : Rann_ack send to nexthop %02x:%02x:%02x:%02x:%02x:%02x\n",rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);
			memcpy(&next,&rt->next_hop,6);
			Make_msg_format_RANN_ACK(&source, &next, rann_ack->alm_next_hop);
		}
	}
	else{
		nms = nms_table_find(source);
		if(!nms){
			printf("Rann_ack receive. But there isn't information about source node\n");
			return NULL;
		}
		else{
			if(rann_ack->alm_next_hop!=0){
				printf("	alm : %u from %02x:%02x:%02x:%02x:%02x:%02x\n",rann_ack->alm_next_hop,source.addr[0],source.addr[1],source.addr[2],source.addr[3],source.addr[4],source.addr[5] );
				nms->alm_next_hop = rann_ack->alm_next_hop;
				memcpy(&nms->last_rann_ack_time,&now,sizeof(struct timeval));
				
				rt = rt_table_find(source,0,source);
				if(!rt){
					printf("Rann_ack receive. But there is no route to root\n");
				}
				else if(rt->hcnt!=1){
					rt->alm_next_hop = nms->alm_next_hop;
				}
			}
		}
	}
}

void Preq_process(uint8_t * pBuf)
{
	struct rt_table *rt;
	PREQ *preq;
	AODV_ext *ext,*ext1,*ext2;

	struct nms_table *nms;

	int i,index,result=0;	
	ll_addr dest, next, source, neighbor, ext_addr1, ext_addr2;
	struct timeval now;


	preq = (PREQ *) pBuf;
	ext = (AODV_ext *) ((char *)preq + sizeof(PREQ));

	int rcv_len = sizeof(PREQ)+(sizeof(AODV_ext)*preq->hcnt);
	//printf("\n preq->hcnt : %d, len: %d \n", preq->hcnt, rcv_len);
	// print_d(pBuf,rcv_len);

	memcpy(&source,my_mac,6);
	memcpy(&neighbor,src_mac_addr,6);

	rt = rt_table_find(neighbor,0,neighbor);

	if(rt == NULL)
		printf("rt is NULL\n");
	else
		preq->alm = preq->alm + rt->alm;
	
	printf("Recv : Preq _ From  %02x:%02x:%02x:%02x:%02x:%02x\n",
	src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],
	src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);

	gettimeofday(&now, NULL);
	
    int ret = -99;
	ret = memcmp(my_mac, preq->dest_addr,6);




	if(ret == 0)	//If I'm Root
	{
		print_d(pBuf,rcv_len);
		if(nms_log_on==true)
			NMS_logging(rcv_len,pBuf);

	    printf("Compare my mac & preq->dest, I am dest\n");
		memcpy(&dest,preq->orig_addr,6);
		memcpy(&next,src_mac_addr,6);

		if(perr_rann_timer.used)
			if(memcmp(&dest,&perr_rann_node,6)== 0)
				perr_rann_check =1;

		rt = rt_table_find(dest,0,dest);


		if (!rt) {
			// index = rt_table_insert(dest, next, source, 0, source , preq->hcnt,preq->orig_IP,0,&now,((float)preq->alm/1000)/(float)preq->hcnt,preq->seqno);
			index = rt_table_insert(dest, next, source, 0, source , preq->hcnt,preq->orig_IP,0,&now,preq->alm,preq->seqno);
			tmp_preq[index].used = true;
			memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
			memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
			
			Prep_start(index);
		} 
		else {
			if(rt->hcnt!=1){
				index = rt_table_get_index(dest);
				if(metric==1 && index >= 0)		// Metric : Hop count
				{
					if(rt->hcnt>preq->hcnt)
					{
						printf("New route is better than origin route\n");
						if(tmp_preq[index].used==true)
						{
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							//add
							//Prep_start(index);

						}
						else
						{
							tmp_preq[index].used = true;
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							Prep_start(index);
						}
					}
					else if(rt->seqno<preq->seqno)
					{
						if(tmp_preq[index].used==false)
						{
							printf("rt_seq (%d), preq_seq(%d) Re Send PREP\n", rt->seqno, preq->seqno);

							tmp_preq[index].used = true;
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							Prep_start(index);
						}
					}	
					else
					{
						printf("Origin route is better than new one\n");
					}
				}
				else if(metric==2 && index >= 0)	// Metric : Airtime Link Metric
				{		
					if(preq->alm>rt->alm)
					{
						printf("New route is better than origin route\n");
						if(tmp_preq[index].used==true)
						{
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							//add
							//Prep_start(index);
						}
						else
						{
							tmp_preq[index].used = true;
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							Prep_start(index);
						}
					}
					else if(rt->seqno<preq->seqno)
					{
						if(tmp_preq[index].used==false)
						{
							printf("Re Send PREP\n");

							tmp_preq[index].used = true;
							memcpy(&(tmp_preq[index].t_preq),preq,sizeof(PREQ));
							memcpy(&(tmp_preq[index].t_ext),ext,sizeof(AODV_ext)*preq->hcnt);
							rt_table_update(rt, next, preq->hcnt,&now,preq->alm,preq->seqno);
							Prep_start(index);
						}
					}	
					else
					{
						printf("Origin route is better than new one\n");
					}
				}
				else
					printf("Metric is not 1 or 2 or index is null\n");
			}
		}	

		if(preq->hcnt==1)
		{
			nms = nms_table_find(dest);
			if(!nms){
				nms_table_insert(dest,source,preq->r_ch, 1,0);
			}
		}

		for(i=0;i<preq->hcnt-1;i++){
			ext1 = (AODV_ext *) ((char *)ext + sizeof(AODV_ext)*(i));
			ext2 = (AODV_ext *) ((char *)ext + sizeof(AODV_ext)*(i+1));

			memcpy(&ext_addr1, ext1->addr,6);
			memcpy(&ext_addr2, ext2->addr,6);


			nms = nms_table_find(ext_addr1);
			if(!nms){
				if(i==0){
					printf("test1 insert!!!\n");
					nms_table_insert(ext_addr1,ext_addr2,preq->r_ch, 1,0);
				}
				else{
					printf("test2 insert!!!\n");
					nms_table_insert(ext_addr1,ext_addr2, 0, 1,0);
				}	
			}
			else{
				if(i==0)
					nms_table_update(nms, ext_addr2, preq->r_ch, 1,0);
				else
					nms_table_update(nms, ext_addr2, 0, 1,0);
			}
		
		
			nms = nms_table_find(ext_addr2);
			if(!nms){
				printf("test3 insert!!!\n");
				nms_table_insert(ext_addr2, ext_addr1, 0, 1,0);
			}
			else{
				printf("test4 insert!!!\n");
				nms_table_update(nms, ext_addr1, 0, 1,0);
			}
		}

	}	
	else
	{
		for(i=0;i<preq->hcnt;i++)
		{
			ext = (AODV_ext *) ((char *)ext + sizeof(AODV_ext)*i);

			if(!memcmp(my_mac,&ext->addr,6))
			{	
				result = 1;
				break;
			}
		}
		if(result == 1)
			printf("This msg is dropped : Repeated Msg\n");
		else{
			if(nms_log_on==true)
				NMS_logging(rcv_len,pBuf);
			Preq_forward(preq);
		}
	}
}

void Prep_process(unsigned char *pBuf)
{
	PREP *prep;
	AODV_ext *ext;
	
	ll_addr dest, next, source, prev;
	struct timeval now;
	struct rt_table *rt;

	uint8_t seqno = 0;

	prep = (PREP *) pBuf;
	ext = (AODV_ext *) ((char *)prep + sizeof(PREP));

	printf("Recv : Prep _ From %02x:%02x:%02x:%02x:%02x:%02x ext-len: %d\n",
	    src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],
		src_mac_addr[3],src_mac_addr[4],src_mac_addr[5], prep->ext_length);
	
	seqno = prep->seqno;

	if(nms_log_on==true)
		NMS_logging((sizeof(PREP)+prep->ext_length*sizeof(AODV_ext)),pBuf);

//Route to ROOT
	memcpy(dest.addr,(uint8_t *)prep->orig_addr,6);
	memcpy(next.addr,(uint8_t *)src_mac_addr,6);
	memcpy(source.addr,(uint8_t *)prep->dest_addr,6);
	if(prep->ext_length>0)
	{
		ext = (AODV_ext *)((char *)ext+sizeof(AODV_ext)*(prep->ext_length-1));
		memcpy(prev.addr,(uint8_t *)ext->addr,6);
	}
		

	gettimeofday(&now, NULL);

	rt = rt_table_find(dest,1,source);

	//UPlink
	if (!rt && memcmp(my_mac,&dest, sizeof(ll_addr))  != 0) {
		rt_table_insert(dest, next, source, prep->ext_length, prev, prep->hcnt,prep->orig_IP,0,&now,0.0,seqno);
	} 
	else{
		rt_table_update(rt, next, prep->hcnt, &now, 0, seqno);
	}

	prep->hcnt++;

//PREQ Sender != ME
	if(prep->ext_length>0)
	{
		//Route to PREQ Sender
		ext = (AODV_ext *)((char *)ext+sizeof(AODV_ext)*(prep->ext_length-1));
		
		memcpy(dest.addr,(uint8_t *)prep->dest_addr,6);
		memcpy(next.addr,(uint8_t *)ext->addr,6);
		memcpy(source.addr,(uint8_t *)prep->orig_addr,6);
		memcpy(prev.addr,(uint8_t *)src_mac_addr,6);

		rt = rt_table_find(dest,0,dest);

		//Downlink
		if(!rt){
			rt_table_insert(dest, next, source, 1, prev, prep->ext_length,prep->dest_IP,0,&now,0.0,0);
		}

		Prep_forward(prep);		
	}
}

void Perr_process(uint8_t * pPayload)
{
	PERR *perr;
	int i, ret = -99;

	//print_d (pPayload, sizeof(PERR));

	perr = (PERR *) pPayload;
	ll_addr source;
	bool sent = false;

	memcpy(&source,my_mac,6);

	printf("Recv : Perr _ From %02x:%02x:%02x:%02x:%02x:%02x\n",
	    src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],
		src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);

	printf("		Perr msg : timeout %02x:%02x:%02x:%02x:%02x:%02x \n", 
	    perr->timeout_addr[0],perr->timeout_addr[1],perr->timeout_addr[2],
		perr->timeout_addr[3],perr->timeout_addr[4],perr->timeout_addr[5]);
	
	
	if(nms_log_on==true)
		NMS_logging(sizeof(PERR),pPayload);
	
	for(i=0;i<10;i++)
	{
		sent = false;

		//PERR전달해준 노드는 uplink에 문제가 없다. 
		if(!memcmp(perr->timeout_addr,perr->dest_addr,6)){
			if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].dest_addr,6))
			{
				printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
				    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
					p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
				
				#if 0
				memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
				p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
				p_rt_list->rt_t[i].hcnt = 0;
				p_rt_list->rt_t[i].hello_cnt = 0;
				p_rt_list->rt_t[i].state = 0;	//empty	
				#endif
				rt_table_empty(i);
				continue;
			}
			if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].next_hop,6))
			{
				printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
				    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
					p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
				
				#if 0
				memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
				p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
				p_rt_list->rt_t[i].hcnt = 0;
				p_rt_list->rt_t[i].hello_cnt = 0;
				p_rt_list->rt_t[i].state = 0;	//empty	
				#endif
				rt_table_empty(i);
				continue;
			}
			if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].source_addr,6))
			{
				printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
				    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
					p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
				
				#if 0
				memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
				p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
				p_rt_list->rt_t[i].hcnt = 0;continue;
				p_rt_list->rt_t[i].hello_cnt = 0;
				p_rt_list->rt_t[i].state = 0;	//empty	
				#endif
				rt_table_empty(i);
				continue;
			}
			if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].prev_addr,6))
			{
				printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
				    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
					p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
				
				#if 0
				memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
				p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
				p_rt_list->rt_t[i].hcnt = 0;
				p_rt_list->rt_t[i].hello_cnt = 0;
				p_rt_list->rt_t[i].state = 0;	//empty	
				#endif
				rt_table_empty(i);
				continue;
			}
		}//PERR전달해준 노드의 uplink broken
		else{
			if(!memcmp(perr->dest_addr,&p_rt_list->rt_t[i].next_hop,6)){
				if(!memcmp(&root_addr,&p_rt_list->rt_t[i].dest_addr,6)){
					printf("Perr : Uplink is broken. The next hop %02x:%02x:%02x:%02x:%02x:%02x is deleted.\n",
					    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
						p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
					if(sent==false){
						Perr_send(&perr->timeout_addr, &source);
						sent=true;
					}
					rt_table_empty(i);
					continue;
				}
			}
			else{
				if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].dest_addr,6))
				{
					printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
					    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
						p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
					
					#if 0
					memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
					p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
					p_rt_list->rt_t[i].hcnt = 0;
					p_rt_list->rt_t[i].hello_cnt = 0;
					p_rt_list->rt_t[i].state = 0;	//empty	
					#endif
					rt_table_empty(i);
					continue;
				}
				if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].next_hop,6))
				{
					printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
					    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
						_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
					
					#if 0
					memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
					p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
					p_rt_list->rt_t[i].hcnt = 0;
					p_rt_list->rt_t[i].hello_cnt = 0;
					p_rt_list->rt_t[i].state = 0;	//empty	
					#endif
					rt_table_empty(i);
					continue;
				}
				if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].source_addr,6))
				{
					printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
					    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
						p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
					
					#if 0
					memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
					p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
					p_rt_list->rt_t[i].hcnt = 0;continue;
					p_rt_list->rt_t[i].hello_cnt = 0;
					p_rt_list->rt_t[i].state = 0;	//empty	
					#endif
					rt_table_empty(i);
					continue;
				}
				if(!memcmp(perr->timeout_addr,&p_rt_list->rt_t[i].prev_addr,6))
				{
					printf("Perr : %02x:%02x:%02x:%02x:%02x:%02x route is deleted.\n",
					    p_rt_list->rt_t[i].dest_addr.addr[0],p_rt_list->rt_t[i].dest_addr.addr[1],p_rt_list->rt_t[i].dest_addr.addr[2],
						p_rt_list->rt_t[i].dest_addr.addr[3],p_rt_list->rt_t[i].dest_addr.addr[4],p_rt_list->rt_t[i].dest_addr.addr[5]);
					
					#if 0
					memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
					p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
					p_rt_list->rt_t[i].hcnt = 0;
					p_rt_list->rt_t[i].hello_cnt = 0;
					p_rt_list->rt_t[i].state = 0;	//empty	
					#endif
					rt_table_empty(i);
					continue;
				}
			}
		}
	}
}

void Perr_rann_mechanism(ll_addr timeout_addr)
{	
	struct nms_table *nms;

	//preq 응답 X  => 노드 죽음으로 판정
	if(perr_rann_check == 0){
		printf("perr_rann_mechanism timer is over. The timeout node dead\n");

		nms = nms_table_find(perr_rann_node);
		nms->node_state = 0;
	}

	Timer_remove(&perr_rann_timer);
	perr_rann_check = 0;
	memset(&perr_rann_node.addr[0],0,6);

}


//Timer 만들어서 preq 안들어오면 노드 죽었다고 판단.
void Perr_rann_mechanism_timer(ll_addr timeout_addr)
{	
	if(perr_rann_timer.used)
	{
		printf("Already rann_timer is worked!\n");
	}
	else{
		memcpy(&perr_rann_node,&timeout_addr.addr[0],6);
		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&perr_rann_timer, (void *) &Perr_rann_mechanism, NULL);
		Timer_set_timeout(&perr_rann_timer, PERR_RANN_MECHANISM_INTERVAL);
	}

}

//Multi version
// void Perr_unicast_state_process(uint8_t * pPayload)
// {
// 	//Perr_unicast의 sender가 죽은 노드의 Uplink라면 => 링크의 죽음으로 처리 & Timer 두어서 일정 time 동안 preq가 오지 않으면 노드의 죽음으로 처리 
// 	//Perr_unicast의 sender가 죽은 노드의 Uplink가 아니라면 => 링크의 죽음으로 처리 -> 서로 이웃노드 해제 & 해당 링크를 타고오는 uplink 해제 
// 	PERR_UNI *perr;
// 	ll_addr	timeout_addr,orig_addr;
// 	struct nms_table *nms;
// 	int i = -1;

// 	perr = (PERR_UNI *) pPayload;

// 	memcpy(&timeout_addr.addr[0],&perr->timeout_addr[0],6);
// 	memcpy(&orig_addr.addr[0],&perr->orig_addr[0],6);

// 	nms_table_perr_update(timeout_addr,orig_addr);

// 	nms = nms_table_find(timeout_addr);
// 	i = nms_table_uplink_find(nms,orig_addr);

// 	//orig_addr is uplink of timeout_addr ==>> 1
// 	if(i==1)
// 		Perr_rann_mechanism_timer(timeout_addr);
// }

void Perr_unicast_state_process(uint8_t * pPayload)
{
	PERR_UNI *perr;
	ll_addr	timeout_addr,orig_addr;
	struct nms_table *nms;
	int i = -1;

	perr = (PERR_UNI *) pPayload;

	memcpy(&timeout_addr,perr->timeout_addr,6);
	memcpy(&orig_addr,perr->orig_addr,6);

	nms_table_perr_update(timeout_addr,orig_addr);
}


void Perr_unicast_process(uint8_t * pPayload)
{
	PERR_UNI *perr;
	int i, ret = -99;

	//print_d (pPayload, sizeof(PERR));

	perr = (PERR_UNI *) pPayload;

	printf("Recv : Perr_unicast _ From %02x:%02x:%02x:%02x:%02x:%02x\n",
	    src_mac_addr[0],src_mac_addr[1],src_mac_addr[2],
		src_mac_addr[3],src_mac_addr[4],src_mac_addr[5]);

	// NMS_logging(sizeof(PERR),pPayload);
	
	if(RootNode == 1)
		Perr_unicast_state_process(pPayload);
	else
		Perr_forward(pPayload);

}


void Data_process(uint8_t * pPayload)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";

	DATA *data;
	char * msg;	

	data = (DATA *)pPayload;
	memcpy(tbuff, pPayload, data->size+sizeof(DATA));	
	//print_d(tbuff, data->size+sizeof(DATA));


	msg =  (char *) ((char *)tbuff + sizeof(DATA));
	struct iphdr *ip = (struct iphdr *) msg;

	//print_d(msg, data->size);

	memcpy (&pIPH->SrcIP, &ip->saddr, 4);
	memcpy (&pIPH->DstIP, &ip->daddr, 4);

	printf("Recv : Data _ From: %i.%i.%i.%i/size : %d \n", 
	    (pIPH->SrcIP) & 0xFF, ((pIPH->SrcIP) >> 8) & 0xFF, 
		((pIPH->SrcIP) >> 16) & 0xFF, ( (pIPH->SrcIP)>>24) & 0xFF, data->size);
	

	if(!memcmp(&my_ip,&pIPH->DstIP,4))
	{
		Data_send_dst(msg,data->size);
	}
	else
	{
		Data_forward(msg,pIPH->DstIP,data->size);
	}
}

void Data_process2(uint8_t * pPayload)
{
	uint8_t tbuff[MAX_FRAME_LEN]="";	DATA *data;
	char * msg;	

	data = (DATA *)pPayload;
	memcpy(tbuff, pPayload, data->size+sizeof(DATA));
	msg= (char*)((char*)pPayload +sizeof(DATA));	

	if(nms_log_on==true)
		NMS_logging(data->size+sizeof(DATA),pPayload);

	Data_start2(&src_mac_addr, msg, data->size);
}

void NMS_log_process(uint8_t * pPayload)
{
	printf("\nIn NMS_log_process!!!\n");

	NMSLOGINFO *nms_str;
	RANN* msg;	

	ll_addr source;

	nms_str = (NMSLOGINFO *)pPayload;
	msg = (RANN *)((char *)pPayload+sizeof(NMSLOGINFO));

	memcpy(&source,nms_str->orig_addr,6);

	// printf("sizeof RANN : %u\n",sizeof(RANN));
	// printf("nmsg_str->size : %u\n",nms_str->size);
	// printf("nmsg_str->type : %u\n",nms_str->type);
	// printf("nmsg_str->addr : %02x:%02x:%02x:%02x:%02x:%02x\n",nms_str->orig_addr[0],nms_str->orig_addr[1],nms_str->orig_addr[2],nms_str->orig_addr[3],nms_str->orig_addr[4],nms_str->orig_addr[5]);

	// printf("msg->type = %u\n",msg->type);
	// printf("msg->addr= %02x:%02x:%02x:%02x:%02x:%02x\n",msg->orig_addr[0],msg->orig_addr[1],msg->orig_addr[2],msg->orig_addr[3],msg->orig_addr[4],msg->orig_addr[5]);
	// printf("msg->order = %u\n",msg->order);

	if(RootNode == 1) {
		// printf("\nIn process contents : %d !!!\n",(int)msg);
		NMS_log_send_nms(nms_str,msg,nms_str->size);	
	}
	else{
		NMS_log_forward(pPayload,nms_str->size,source);
	}
}

void rt_NMS_reset(){
	printf("in rt_nms\n");
	if(RootNode==1){
		nms_table_empty(12);
		nms_table_print();
	}
	rt_table_empty(10);
	rt_table_print();
}


//receiving channel 변경시 ->  RT,NMS 변경
//그 외의 상황 -> RT, NMS 초기화
void nms_control_msg_process(char *control_msg)
{
	int rann_bit=0,reset=0;
	struct nms_table *nms;
	ll_addr node,reset_addr,blank;

	

	NMSCONTROL *nmsmsg;
  	NMSCONTROLHDR *nmsmsgtype;
	
	nmsmsgtype = (NMSCONTROLHDR *)control_msg;

	memset(&blank, 0, 6);

	// printf("type : %u / value : %u \n",nmsmsgtype->type,nmsmsgtype->value);


	switch(nmsmsgtype->type){
		//Case 0 : change metric, recv_ch, data_rate ( global )
        case 0:
			nmsmsg = (NMSCONTROL *)control_msg;
			memcpy(&node, my_mac,6);
			nms = nms_table_find(node);

			if(nms){
				printf("[In NMS-control_msg_process] : (now) metric=%u, data_rate=%u, recv_channel=%u\n",metric, nms->data_rate ,nms->receiving_channel);
				printf("                               (new) metric=%u, data_rate=%u, recv_channel=%u\n",nmsmsg->metric, nmsmsg->data_rate, nmsmsg->recv_channel);
			}
			else{
				printf("[In NMS-control] : nms table error!!\n");
				return NULL;
			}
			
			//Receiving channel 변경 여부 확인
			if(receiving_channel!=nms->receiving_channel){
				nms->receiving_channel = receiving_channel;
				printf("[In NMS-control] : recv_channel is changed\n");	
				rt_NMS_reset();
			}	

			//나머지 상황은 모두 rt, NMS 초기화
			//Data rate 변경 여부 확인
			if(data_rate!=nms->data_rate){
				nms->data_rate = data_rate;
				printf("[In NMS-control] : data_rate is changed\n");	
				rt_NMS_reset();
			}

			//Metric 변경 여부 확인
			if(metric != nmsmsg->metric){
				metric = nmsmsg->metric;
				nms->metric = metric;
				printf("[In NMS-control] : metric is changed\n");	
				rt_NMS_reset();
			}
		break;

		//Case 1 : on/off about log  (0 : keep / 1 : off / 2 : on)
		case 1:
			//log change
			if(nmsmsgtype->value == 2)
				nms_log_on = true;
			else
				nms_log_on = false;
		break;

		//Case 2 : reset route table (0 : no reset / 1 : reset)
        case 2:
			//경로 재설정 확인
			if(nmsmsgtype->value == 1){
				reset++;
				printf("[In NMS-control] : All route reset\n");	
			}
			if(reset>0){
				rt_NMS_reset();
				route_reset=1;
				reset=0;
			}
		break;	

        //Case 3 : change update cycle (0 : default = 1sec X 1 / 1 : often = 1sec X 0.5 / 2 : less often = 1sec X 1.5)
        case 3:
		  	printf("[In NMS-control] : Update period is changed to %f \n",update_cycle);
			Make_msg_format_RANN(&blank);
			Timer_set_timeout(&rann_timer, (long)RANN_INTERVAL*update_cycle);
        break;

		//Case 99 : reboot (reboot addr)
        case 99:
          	memcpy(&reset_addr,&nmsmsgtype->value,6);
			rann_order++;
			Make_msg_format_RANN(&reset_addr);
			Timer_set_timeout(&rann_timer, (long)RANN_INTERVAL*update_cycle);
        break;
	}
}

void API_reset_process(void){
	if(api_reset==1){
		Reset();
		api_reset=0;
	}
	else if(reboot_flag==1){
		Program_reboot();
	}
}

void API_reset_timer_start(void)
{
	printf("In API reset timer\n");

	if(API_reset_timer.used)
	{
		printf("Already API reset timer is worked!\n");
	}
	else
	{
		gettimeofday(&this_host.fwd_time,NULL);
		Timer_init(&API_reset_timer,(void *) &API_reset_process, NULL);
	}
}

void Receive_process(uint8_t * pPayload)
{
    int m_type = ((AODV_msg *) pPayload)->type;	

    switch(m_type)
    {
	case 0: //hello receive
		Hello_process(pPayload);
	break;

	case 1: //rann receive, rann forward
		Rann_process(pPayload);
	break;
	

	case 2:
		Preq_process(pPayload);			
	break;

	case 3:
		Prep_process(pPayload);			
	break;
	case 4:
		Data_process(pPayload);	
	break;

	case 5:
		Perr_process(pPayload);
	break;

	case 6:
		NMS_log_process(pPayload);
	break;

	case 7:
		Perr_unicast_process(pPayload);
	break;

	case 8:
		if(print_time == 0){
			print_time = 2;		
			// WSM_Print2(__FUNCTION__);
		
			Data_process2(pPayload);
		}
		else print_time = 0; 
	break;

	case 9:	//SCH Hello receive
		SCH_Hello_process(pPayload);
	break;

	case 10:	//Rann_ack receive
		Rann_ack_process(pPayload);
	break;

	default:
	break;
    }	

}

void rt_table_empty(int index)
{
    int i = 0;

    if(index == 10)
    {
	for(i=0;i<10;i++)
	{
		memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
		p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
		p_rt_list->rt_t[i].hcnt = 0;
		p_rt_list->rt_t[i].hello_cnt = 0;
		p_rt_list->rt_t[i].sch_hello_cnt = 0;
		p_rt_list->rt_t[i].alm = 0; 
		p_rt_list->rt_t[i].dest_IP = 0; 
		p_rt_list->rt_t[i].seqno = 0; 
		p_rt_list->rt_t[i].state = 0;	//empty
		p_rt_list->rt_t[i].alm_buffer_pointer = 0;
		p_rt_list->rt_t[i].sch_hello_detector = 0;

		tmp_preq[i].used = false;

	}
    } else if ( index > 10 || index < 0)
	printf("Invaild rt_table_index 0~10\n");
    else 
    {
		memset(&p_rt_list->rt_t[index], 0, sizeof(struct rt_table));
		p_rt_list->rt_t[index].ifindex = if_nametoindex("wave-data0");
		p_rt_list->rt_t[index].hcnt = 0;
		p_rt_list->rt_t[index].hello_cnt = 0;
		p_rt_list->rt_t[index].sch_hello_cnt = 0;
		p_rt_list->rt_t[index].alm = 0; 
		p_rt_list->rt_t[index].dest_IP = 0; 
		p_rt_list->rt_t[index].seqno = 0; 
		p_rt_list->rt_t[index].state = 0;	//empty
		p_rt_list->rt_t[index].alm_buffer_pointer = 0;
		p_rt_list->rt_t[index].sch_hello_detector = 0;

		tmp_preq[index].used = false;

    }	

}

void rt_table_init(struct rt_table_list *p_rt_list)
{
	int i=0;

	#if 0
	for(i=0;i<10;i++)
	{
		memset(&p_rt_list->rt_t[i], 0, sizeof(struct rt_table));
		p_rt_list->rt_t[i].ifindex = if_nametoindex("wave-data0");
		p_rt_list->rt_t[i].hcnt = 0;
		p_rt_list->rt_t[i].hello_cnt = 0;
		p_rt_list->rt_t[i].state = 0;	//empty
	}
	#endif

	rt_table_empty(10);

//	printf("rt_table_init!\n");

	//add ETRI, neuron, 2020.08 for IP address
	//unsigned char  ipv4_addr[IPv4_ADDR_STR_LEN];
	uint8_t dest_IP[4] = {0xff, 0xff, 0xff, 0xff};
	static tIPH IP;
  	pIPH= &IP;

	//memset(ipv4_addr, 0, IPv4_ADDR_STR_LEN);
	//i = mesh_get_ipv4_address(NETIFNAME, ipv4_addr);

	//my_ip = inet_addr(ipv4_addr);
	pIPH->SrcIP = my_ip;
	pIPH->DstIP = (dest_IP[0] << 24)+ (dest_IP[1] << 16) + (dest_IP[2] << 8) + dest_IP[3];

  	//printf("---- my IP address  s:  %i.%i.%i.%i \n", my_ip & 0xFF, (my_ip >> 8) & 0xFF, (my_ip >> 16) & 0xFF, ( my_ip>>24) & 0xFF);
  	
	i = mesh_get_mac_address(NETIFNAME, my_mac);
	memcpy(src_mac_addr, my_mac, _ETHER_ADDR_LEN);

	printf("---- my MAC address s:%02x:%02x:%02x:%02x:%02x:%02x \n", my_mac[0], my_mac[1],
		my_mac[2], my_mac[3], my_mac[4], my_mac[5]);

	for(i = 0; i < _ETHER_ADDR_LEN; i++)
		dest_mac_addr[i] = 0xff;
	
	// if(RootNode == 1)
		nms_table_empty(12);

	Init_Q();
}
	
struct rt_table *rt_table_find(ll_addr dest_addr, uint8_t source_consider ,ll_addr source_addr)
{
	int i = 0;

	if(source_consider == 0){
		for(i=0;i<10;i++){
			if (memcmp(&dest_addr, &p_rt_list->rt_t[i].dest_addr, 6) == 0 && (p_rt_list->rt_t[i].state == 1)){
				return &(p_rt_list->rt_t[i]);
			}
		}
	}
	else{
		for(i=0;i<10;i++){
			if (memcmp(&dest_addr, &p_rt_list->rt_t[i].dest_addr, 6) == 0 && memcmp(&source_addr, &p_rt_list->rt_t[i].source_addr, 6) == 0 && (p_rt_list->rt_t[i].state == 1)){
				return &(p_rt_list->rt_t[i]);
			}
		}
	}
	return NULL;
}

struct rt_table *rt_table_find_IP(uint32_t IPaddr)
{
	int i = 0;
	for(i=0;i<10;i++)
		if (memcmp(&IPaddr, &(p_rt_list->rt_t[i].dest_IP), sizeof(IPaddr)) == 0 && (p_rt_list->rt_t[i].state == 1))
			return &(p_rt_list->rt_t[i]);
	return NULL;
}

int rt_table_get_index(ll_addr dest_addr)
{
	int i = 0, ret = -1;
	for(i=0;i<10;i++)
		if (memcmp(&dest_addr, &p_rt_list->rt_t[i].dest_addr, 6) == 0 && (p_rt_list->rt_t[i].state == 1))
			ret = i;
	return ret;
}

void rt_table_update(struct rt_table *rt, ll_addr next, uint8_t hcnt, struct timeval *now,uint32_t alm, uint32_t seqno)
{
	memcpy(&rt->next_hop,&next,6);
	memcpy(&rt->last_hello_time,&now,sizeof(struct timeval));
	rt->hcnt = hcnt;
	rt->alm = alm;
	rt->state = 1;
	rt->seqno = seqno;

	printf("rt_table_update is finish!\n");	
	
	printf("Route update\n Dest : %02x:%02x:%02x:%02x:%02x:%02x     next hop : %02x:%02x:%02x:%02x:%02x:%02x\n", rt->dest_addr.addr[0],rt->dest_addr.addr[1],rt->dest_addr.addr[2],rt->dest_addr.addr[3],rt->dest_addr.addr[4],rt->dest_addr.addr[5],rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);

}

//dest_addr, next_hop, source, prev, hop_count, IP_addr, hello_count, timer, alm, seqnum
int rt_table_insert(ll_addr dest_addr, ll_addr next, ll_addr source_addr, uint8_t prev_exist, ll_addr prev_addr, uint8_t hcnt, uint32_t IPaddr, uint32_t hello_cnt, struct timeval *now, uint32_t alm, uint32_t seqno)
{
	struct rt_table *rt;
	int i;
	
	for(i=0;i<11;i++){
		if(p_rt_list->rt_t[i].state==0)
			break;
	}
	if(i==11)
	{
		printf("Routing Table is full!!!!\n");
		return -1;
	}
	rt = &(p_rt_list->rt_t[i]);

	memcpy(&p_rt_list->rt_t[i].dest_addr,&dest_addr,6);
	memcpy(&p_rt_list->rt_t[i].next_hop,&next,6);
	memcpy(&p_rt_list->rt_t[i].source_addr,&source_addr,6);
	if(prev_exist > 0)
		memcpy(&p_rt_list->rt_t[i].prev_addr,&prev_addr,6);
	memcpy(&p_rt_list->rt_t[i].first_hello_time,now,sizeof(struct timeval));
	memcpy(&p_rt_list->rt_t[i].last_hello_time,now,sizeof(struct timeval));
	rt->hcnt = hcnt;
	rt->state = 1;
	rt->hello_cnt = hello_cnt;
	rt->dest_IP = IPaddr;
	rt->alm = alm;
	rt->seqno = seqno;

	//p1 = (unsigned char*)&rt->dest_IP;
	
	printf("rt_table_insert is finish!\n");	
	
	printf("Route insert %d Dest : %02x:%02x:%02x:%02x:%02x:%02x     next hop : %02x:%02x:%02x:%02x:%02x:%02x\n", i, rt->dest_addr.addr[0],rt->dest_addr.addr[1],rt->dest_addr.addr[2],rt->dest_addr.addr[3],rt->dest_addr.addr[4],rt->dest_addr.addr[5],rt->next_hop.addr[0],rt->next_hop.addr[1],rt->next_hop.addr[2],rt->next_hop.addr[3],rt->next_hop.addr[4],rt->next_hop.addr[5]);

	rt_table_print();

	return i;
}

void nms_table_empty(int index)
{
    int i = 0;
	
    if(index == 12){
		for(i=0;i<12;i++){
			memset(&p_nms_list->nms_t[i],0,118);
			// p_nms_list->nms_t[i].node_state = 0;
			// p_nms_list->nms_t[i].receiving_channel = receiving_channel;
		}
		memcpy(&p_nms_list->nms_t[0].addr,my_mac,6);
		p_nms_list->nms_t[0].node_state = 1;
		p_nms_list->nms_t[0].receiving_channel = receiving_channel;
		p_nms_list->nms_t[0].data_rate = data_rate;
		p_nms_list->nms_t[0].metric = metric;
		p_nms_list->nms_t[0].alm_next_hop = 0;
    } 
	else if ( index > 12 || index < 0)
		printf("Invaild rt_table_index 0~10\n");
    else 	//PERR 처럼 buffer 비우는 용도 
		memset(&p_nms_list->nms_t[index], 0, sizeof(struct rt_table));
}

struct nms_table* nms_table_find(ll_addr addr)
{
	int i = 0;
	for(i=0;i<12;i++){
		if (memcmp(&addr, &p_nms_list->nms_t[i].addr, 6) == 0 ){
			return &(p_nms_list->nms_t[i]);
		}
	}
	return NULL;
}

void nms_table_perr_update(ll_addr timeout_addr, ll_addr orig_addr){
	struct nms_table *nms;
	int i,j;
	char tmp[48];


	//죽은 노드 nms_table 상태 반영
	nms = nms_table_find(timeout_addr);
	if(!nms){
		printf("There isn't nms_table of perr node\n");
	}
	else{
		memset(&nms->neighbor_node,0,48);
		nms->node_state = 0;
		memset(&nms->uplink_path,0,48);
	}

	//죽은 노드와 관련된 nms_table 상태 반영
	for(i=0;i<12;i++){
		for(j=0;j<8;j++){
			//uplink가 관련 되어 있으면 아예 삭제 
			if(memcmp(&p_nms_list->nms_t[i].uplink_path[j], &timeout_addr,6) == 0){
				memset(&p_nms_list->nms_t[i].uplink_path,0,48);
			}
			//neighbor에 포함되어 있다면 지우고 한칸씩 당기기
			if(memcmp(&p_nms_list->nms_t[i].neighbor_node[j], &timeout_addr,6) == 0){
				memset(&p_nms_list->nms_t[i].neighbor_node[j],0,6);
				if(j!=7){
					memcpy(tmp,&p_nms_list->nms_t[i].neighbor_node[j+1],6*(7-j));
					memcpy(&p_nms_list->nms_t[i].neighbor_node[j],tmp,6*(7-j));
				}

			}
			
		}
	}
}

int nms_table_update(struct nms_table* nms, ll_addr neighbor_addr, uint8_t r_ch, uint8_t state, uint8_t is_hello)
{
	ll_addr blank_addr;
	int i = 0;
	bool same=false;

	nms->node_state = state;

	if(r_ch == 0){
		// printf("set default receiving channel set (172)\n");
		nms->receiving_channel = receiving_channel;
	}	
	else
		nms->receiving_channel = r_ch;
	
	if(is_hello == 1){
		// printf("update!!!!\n");
		memcpy(&nms->uplink_path,nms->addr,6);
	}

	for(i=0;i<4;i++){
		if(memcmp(&mac_list[i][0],&neighbor_addr,6)==0){
			same = true;
			break;
		}
	}

	if(same==true){
		//neighbor로 등록이 되어있는지 확인 -> 등록이 되어 있으면 그냥 스킵 / 등록이 되어 있지 않다면 남은 자리에 추가 
		for(i=0;i<8;i++){
			if (memcmp(&neighbor_addr, &nms->neighbor_node[i], 6) == 0 )
				return i;
		}

		memset(&blank_addr,0,6);

		//neighbor에 빈자리 탐색
		for(i=0;i<8;i++){
			if (memcmp(&blank_addr, &nms->neighbor_node[i], 6) == 0 ){
				memcpy(&nms->neighbor_node[i],&neighbor_addr,6);
				return i;
			}
		}
	}
	else{
		return -1;
	}
}


//addr, neighbor, receiving channel, node state
int nms_table_insert(ll_addr addr, ll_addr neighbor, uint8_t r_ch, uint8_t state, uint8_t is_hello)
{
	struct nms_table *nms;
	int i;
	ll_addr blank;
	bool same=false;

	memset(&blank,0,6);
	
	for(i=0;i<12;i++){
		if(p_nms_list->nms_t[i].metric == 0)
			break;
	}
	if(i==12)
	{
		printf("NMS Table is full!!!!\n");
		return -1;
	}

	for(i=0;i<4;i++){
		if(memcmp(&mac_list[i][0],&addr,6)==0){
			same = true;
			break;
		}
	}
	
	if(same==true){
		nms = &(p_nms_list->nms_t[i]);

		memcpy(&p_nms_list->nms_t[i].addr,&addr,6);

		printf("[In nms_table_insert] : %02x.%02x.%02x.%02x.%02x.%02x\n",nms->addr[0],nms->addr[1],nms->addr[2],nms->addr[3],nms->addr[4],nms->addr[5]);

		//neighbor table 도 돌면서 확인해야할듯....
		memcpy(&p_nms_list->nms_t[i].neighbor_node[0],&neighbor,6);
		p_nms_list->nms_t[i].node_state = state;

		if(r_ch == 0){
			printf("[In nms_table_insert] : input receiving channel is 0. Set default receiving channel %u\n",receiving_channel);
			p_nms_list->nms_t[i].receiving_channel = receiving_channel;
		}
		else
			p_nms_list->nms_t[i].receiving_channel = r_ch;

		p_nms_list->nms_t[i].metric = metric;
		p_nms_list->nms_t[i].data_rate = data_rate;

		// printf("state %d / recv %d / metric %d / data %d\n",state,r_ch,metric,data_rate);

		if(is_hello == 1)
			memcpy(&p_nms_list->nms_t[i].uplink_path,&addr,6);

		return i;
	}
	else{
		printf("error!!!!!!!!!!! shit!!!!!!!!!!!!!!!!!!!\n\n");
		return NULL;
	}

	// if(memcmp(&addr,&blank,2)==0)
	// {
	// 	return -1;
	// }
	// if(memcmp(&addr,&blank,1)!=0)
	// {
	// 	return -1;
	// }
}

int nms_table_uplink_find(struct nms_table* nms, ll_addr source)
{
	int i = 0;
	for(i=0;i<8;i++){
		if (memcmp(&nms->uplink_path[i], &source, 6) == 0 )
			return 1;
	}
	return 0;
}

//nms, source, ext, ext_length
int nms_table_uplink_insert(struct nms_table* nms, AODV_ext* ext, uint8_t ext_length)
{
	int i,j;
	ll_addr blank_addr;
	bool same = false;

	memset(&nms->uplink_path,0,48);

	if(ext_length==0)
		memcpy(&nms->uplink_path[0],&nms->addr,6);
	
	for(i=0;i<ext_length;i++)	{
		if(i!=0)
			ext = (AODV_ext *)((char *)ext+sizeof(AODV_ext));
		same = false;
		for(j=0;j<4;j++){
			if(memcmp(&mac_list[j][0],&ext->addr,6)==0){
				same = true;
			}
		}
		if(same==true){
			memcpy(&nms->uplink_path[i],&ext->addr,6);
		}
	}
	return i;
}

void nms_table_print(void)
{
	int i = 0, cnt = 0, j=0;
	char *test;
	printf("\n------------------NMS Table Print--------------------\n");
	printf(" Index	source				receiving_channel		node_state		metric		data_rate		alm(next hop)\n");
	printf(" neighbor_node\n");
	printf(" uplink_path\n");
	printf("-------------------------------------------------------\n");
	for(i = 0;i < 11;i++){
		if(p_nms_list->nms_t[i].addr[0]==0 && p_nms_list->nms_t[i].addr[1]==0 && p_nms_list->nms_t[i].addr[3]==0 && p_nms_list->nms_t[i].addr[4]==0 && p_nms_list->nms_t[i].addr[5]==0 )
			continue;
		cnt ++;
		printf("\n %d	%02x.%02x.%02x.%02x.%02x.%02x		%d				%d			%d			%d		%d\n",cnt ,p_nms_list->nms_t[i].addr[0],p_nms_list->nms_t[i].addr[1],p_nms_list->nms_t[i].addr[2],p_nms_list->nms_t[i].addr[3],p_nms_list->nms_t[i].addr[4],p_nms_list->nms_t[i].addr[5],p_nms_list->nms_t[i].receiving_channel,p_nms_list->nms_t[i].node_state,p_nms_list->nms_t[i].metric,p_nms_list->nms_t[i].data_rate,p_nms_list->nms_t[i].alm_next_hop);
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

	// test = &p_nms_list->nms_t[1].alm;
	// printf(" %02x %02x %02x %02x \n", test[0], test[1], test[2], test[3]);
	// printf("--------------------------------------------------------\n\n");
	
}

void Timer_timeout(struct timeval *now)
{
    LIST(expTQ);
    list_t *pos, *tmp;

    /* Remove expired timers from TQ and add them to expTQ */
    List_foreach_safe(pos, tmp, &TQ) {
		struct timer *t = (struct timer *) pos;

		if (Timeval_diff(&t->timeout, now) > 0)
			break;

		List_detach(&t->l);
		List_add_tail(&expTQ, &t->l);
    }

    /* Execute expired timers in expTQ safely by removing them at the head */
    while (!List_empty(&expTQ)) {
		struct timer *t = (struct timer *) List_first(&expTQ);
		List_detach(&t->l);
		t->used = 0;
		/* Execute handler function for expired timer... */
		if (t->handler) 
			t->handler(t->data);
    }
}
