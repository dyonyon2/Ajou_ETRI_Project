/**
 * @file
 * @brief wsm 어플리케이션 구현 메인 파일
 *
 * WSM(1609.2 미적용)+mesh control message 송수신하는 어플리케이션
 */


// 시스템 헤더 파일
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/netlink.h>
#include <net/ethernet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <net/if.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

// 어플리케이션 헤더 파일
#include "wsm.h"
#include "aodv.h"
#include "mesh_extern.h"

struct MIB g_mib = {0,}; ///< 어플리케이션 관리정보

static pthread_t pHelloRannThread,pNetlinkThread, pCommandThread, pNMS_send_Thread, pNMS_recv_Thread;
static int thr_rc = 0;

static struct sockaddr_nl src_addr, dest_addr;
static struct nlmsghdr *nlh = NULL;
static struct iovec iov;
static int sock_fd;  
static struct msghdr msg;

static int nms_tcp_send_server, nms_tcp_send_client;
static int nms_tcp_recv_server, nms_tcp_recv_client;


NMSLOGINFO *p_nms_log_str;
// static uint8_t nms_log_buff[MAX_FRAME_LEN]="";

static char	*__bar = "-------------------------------------------------------";
static bool main_state = true;

bool nms_log_on = false;


char nms_send_buff[MAX_FRAME_LEN]="";
char nms_recv_buff[MAX_FRAME_LEN]="";


/**
 * @brief 어플리케이션 사용법을 출력한다.
 * @param[in] app_filename 어플리케이션 실행파일명
 */
static void WSM_Usage(char app_filename[])
{
  printf("\n\n Description: Sample application to transmit and receive WSMs using v2x-sw libraries\n");

  //printf("\n Usage: %s trx|rx [OPTIONS]\n", app_filename);
  printf("\n Usage: %s [OPTIONS --metric|--hop]\n", app_filename);
  printf("\n OPTIONS for \"TRANSMIT AND RECEIVE\" operation\n");
  printf("  --if <if_idx>         Set interface index(0~4) to transmit. If not specified, set to %u\n", DEFAULT_IF_IDX);
  printf("  --psid <psid>         Set PSID(in decimal) to transmit or receive. If not specified, set to %u\n", DEFAULT_PSID);
  printf("  --chan <channel>      Set tx channel number. If not specified, set to %u\n", DEFAULT_CHAN_NUM);
  printf("  --rate <datarate>     Set tx datarate(in 500kbps). If not specified, set to %u(%uMbps)\n", DEFAULT_DATARATE, DEFAULT_DATARATE/2);
  printf("  --power <power>       Set tx power level(in dBm). If not specified, set to %d\n", DEFAULT_POWER);
  printf("  --prio <priority>     Set tx user priority(0~7). If not specified, set to %u\n", DEFAULT_PRIORITY);
  printf("  --dst <address>       Set tx destination MAC address. If not specified, set to broadcast(FF:FF:FF:FF:FF:FF)\n");
  printf("  --len <len>           Set tx WSM body length(in octet). If not specified, set to %u\n", DEFAULT_WSM_BODY_LEN);
  printf("  --interval <usec>     Set tx interval(in usec). If not specified, set to %u(%umsec)\n", DEFAULT_TX_INTERVAL, DEFAULT_TX_INTERVAL/1000);
  printf("  --dbg <level>         Set debug message print level. If not specified, set to %u\n", DEFAULT_DBG);
  printf("                            0: nothing, 1: event, 2: message hexdump\n");
  printf("  --root 0|1            Set node setting value. default value is 0\n");
  printf("  --metric 1|2          Set route select method. 1: hop-count, 2: alm \n");
  printf("  --hop 0|1|2|3         Set Hop Count IEU IPv4 address (ex.0:ROOT-192.168.20.12/3:Default-20.16)\n");
  printf("  --ip <ip address>     Set Connect IEU IPv4 address \n");
  printf("  --log 0|1         Set NMS log\n");

 // printf("\n OPTIONS for \"RECEIVE ONLY\" operation\n");
 // printf("  --psid <psid>         Set PSID(in decimal) to receive. If not specified, set to %u\n", DEFAULT_PSID);
  //printf("  --dbg <level>         Set debug message print level. If not specified, set to %u\n", DEFAULT_DBG);
  //printf("                            0: nothing, 1: event, 2: message hexdump\n");

  //printf("\n Example\n");
 // printf("  1) %s trx             Sending periodically and receiving test WSM(psid=%u)\n", app_filename, DEFAULT_PSID);
 // printf("  2) %s rx              Receiving all WSMs in any interface/channel/timeslot\n", app_filename);
  printf("\n\n");
}


/**
 * @brief V2X 라이브러리들을 초기화한다.
 * @retval 0: 성공
 * @retval -1: 실패
 */
static int WSM_InitV2XLibs(void)
{
  printf("Initialize V2X libraries\n");
  int ret;

#ifdef _MSG_PROCESS_LATENCY_TEST_
  Dot3LogLevel dot3_log_level = kDot3LogLevel_Err;
  WalLogLevel wal_log_level = kWalLogLevel_Err;
#else
  Dot3LogLevel dot3_log_level = kDot3LogLevel_Err;
  WalLogLevel wal_log_level = kWalLogLevel_Err;
#endif

  /*
   * dot3 라이브러리를 초기화한다.
   */
  ret = Dot3_Init(dot3_log_level);
  if (ret < 0) {
    printf("Fail to Dot3_Init(): %d\n", ret);
    return -1;
  }
  printf("Success to Dot3_Init()\n");

  /*
   * 접속계층 라이브러리를 오픈하고 패킷수신콜백함수를 등록한다.
   * WAL_Init()이 아닌 WAL_Open()을 호출하는 이유는, 본 프로그램 실행 전에 설정된 채널 접속이 유지되어야 하기 때문이다.
   */
  ret = WAL_Open(wal_log_level);
  if (ret < 0) {
    printf("Fail to WAL_Open(): %d\n", ret);
    Dot3_Release();
    return -1;
  }
  g_mib.if_num = (unsigned int)ret;
  WAL_RegisterCallbackRxMPDU(WSM_ProcessRxMPDUCallback);
  printf("Success to open access library - %d interface supported\n", g_mib.if_num);

  return 0;
}


/**
 * @brief 종료신호 핸들러. WAL_Close()를 호출한다.
 * @param signo 사용되지 않음.
 */
static void WSM_SigHandler(int signo)
{
  (void)signo;
  printf("Program exits...\n");
  if (g_mib.op == kOperationType_trx) {
    WSM_ReleaseTxOperation();
  }
  Dot3_Release();
  WAL_Close();
  close(nms_tcp_send_server);
  close(nms_tcp_send_client);
  close(nms_tcp_recv_server);
  close(nms_tcp_recv_client);
  exit(0);
}

void Program_reboot(void){
  printf("The device will be restarted.....\n");
  WSM_ReleaseTxOperation();
  Dot3_Release();
  WAL_Close();
  close(nms_tcp_send_server);
  close(nms_tcp_send_client);
  close(nms_tcp_recv_server);
  close(nms_tcp_recv_client);
  reboot(RB_AUTOBOOT);
}

void Reset(void){
  int ret=0;

  printf("[In reset] : API Release & Reinit\n");

  WSM_ReleaseTxOperation();
  Dot3_Release();
  WAL_Close();

  g_mib.tx_datarate = data_rate;
  g_mib.tx_chan_num2 = receiving_channel;

  /*
  * V2X 라이브러리들을 초기화한다.uccess to Dot3
  */
  ret = WSM_InitV2XLibs();
  if (ret < 0) {
    printf("Fail to WSM_InitV2XLibs\n");
      exit(0);
  }

  /*
  * 각 인터페이스의 MAC 주소를 확인한다 (WSM MPDU의 MAC 헤더에 수납하기 위해)
  */
  for (unsigned int if_idx = 0; if_idx < g_mib.if_num; if_idx++) {
    ret = WAL_GetIfMACAddress(if_idx, g_mib.my_addr[if_idx]);
    if (ret < 0) {
      printf("Fail to WAL_GetIfMACAddress() for if[%u] - %d\n", if_idx, ret);
      exit(0);
    }
    printf("Check if[%u] MAC address - "MAC_ADDR_FMT"\n", if_idx, MAC_ADDR_FMT_ARGS(g_mib.my_addr[if_idx]));
  }

  /*
  * set channel number using user specified options
  */
  ret = WAL_AccessChannel(g_mib.tx_if_idx, g_mib.tx_chan_num, g_mib.tx_chan_num);//Default : interface 0-178-178(Continuous)

  if (ret < 0)
  {
      printf("Channel number configuration is failed(error=%d) for if=%d ch=%d", ret, g_mib.tx_if_idx, g_mib.tx_chan_num);
      exit(0);
  }

  ret = WAL_AccessChannel(DEFAULT_IF_IDX+1, g_mib.tx_chan_num2, g_mib.tx_chan_num2);//RF2 : interface 1-receiving channel(Continuous)

  if (ret < 0)
  {
      printf("Channel number configuration is failed(error=%d) for if=%d ch=%d/%d", ret, g_mib.tx_if_idx+1, g_mib.tx_chan_num2, g_mib.tx_chan_num2);
      exit(0);
  }

  /*
  * WSM 수신을 위한 PSID를 등록한다.
  */
  ret = Dot3_AddWSR(g_mib.psid);
  if (ret < 0) {
    printf("Fail to add WSR(psid: %u) - %d\n", g_mib.psid, ret);
    exit(0);
  }

  /*
  * 메시지 송신 루틴을 초기화한다.
  */
  if (g_mib.op == kOperationType_trx) {
    ret = WSM_InitTxOperation(g_mib.tx_interval);
    if (ret < 0) {
      printf("WSM_InitTxOperation is failed\n");
      exit(0);
    }
  }
}

void* NMS_TCP_send_func(void)
{
  struct sockaddr_in serv_addr,clnt_addr;
  socklen_t clnt_addr_size;

  int i,cnt=0;
  int reuse = 1;
  int ret = 0;
  // int ack=0;
  // char buf[10];

  nms_tcp_send_server = socket(PF_INET, SOCK_STREAM,0);
  if(nms_tcp_send_server == -1)
    printf("nms_tcp_send_server socket error!\n");

  if(setsockopt(nms_tcp_send_server,SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))==-1)
  {
    printf("socket setsockpty error!");
  }
  
  
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  serv_addr.sin_port=htons(8888);

  if(bind(nms_tcp_send_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1){
    printf("nms_tcp_send_server bind error!\n");
    exit(0);
  }
  
  if(listen(nms_tcp_send_server,5) == -1)
    printf("nms_tcp_send_server listen error!\n");

  clnt_addr_size = sizeof(clnt_addr);
  nms_tcp_send_client = accept(nms_tcp_send_server, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
  if(nms_tcp_send_client == -1)
    printf("nms_tcp_send_server accept error!\n");

  printf("\nnms_tcp_send_server accept clear!!!!!! \n");

  // printf("metric = %d / data_rate = %d\n",metric, data_rate);
      
  while(1){
    NMS_Connection = 1;
    nms_tcp_send_client_ext = nms_tcp_send_client;
    cnt++;
    // printf("send %d \n",cnt);
    for(i=0;i<8;i++){
      memset(nms_send_buff,0,MAX_FRAME_LEN);
      memcpy(nms_send_buff,&p_nms_list->nms_t[i],110);
      ret=send(nms_tcp_send_client,nms_send_buff, 110, 0);
      
      if(ret == -1){
        printf("[In NMS_TCP_send-function] : send error! Try to connect again\n");
        NMS_Connection = 0;
        if(listen(nms_tcp_send_server,5) == -1){
          printf("nms_tcp_send_server listen error!\n");
          exit(0);
        }
        nms_tcp_send_client = accept(nms_tcp_send_server, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(nms_tcp_send_client == -1){
          printf("nms_tcp_send_server accept error!\n");
          exit(0);
        }
        else{
          NMS_Connection = 1;
          printf("\nnms_tcp_send_server accept clear!!!!!! \n");
        }
      }
      // printf("send %d \n",i);
      usleep(500000);
    }
    usleep(1000000 * update_cycle);
  }
}

void* NMS_TCP_recv_func(void)
{
  struct sockaddr_in serv_addr,clnt_addr;
  socklen_t clnt_addr_size;

  NMSCONTROLHDR *nmsmsgtype;
	NMSCONTROL *nmsmsg;

  int byte=0;
  int reuse=1;

  nms_tcp_recv_server = socket(PF_INET, SOCK_STREAM,0);
  if(nms_tcp_recv_server == -1)
    printf("nms_tcp_recv_server socket error!\n");

  if(setsockopt(nms_tcp_recv_server,SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))==-1)
    printf("socket setsockpty error!");
  
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  serv_addr.sin_port=htons(8889);

  if(bind(nms_tcp_recv_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    printf("nms_tcp_recv_server bind error!\n");
  
  if(listen(nms_tcp_recv_server,5) == -1)
    printf("nms_tcp_recv_server listen error!\n");

  clnt_addr_size = sizeof(clnt_addr);
  nms_tcp_recv_client = accept(nms_tcp_recv_server, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
  if(nms_tcp_recv_client == -1)
    printf("nms_tcp_recv_server accept error!\n");

  printf("\nnms_tcp_recv_server accept clear!!!!!! \n");
   
  while(1){
    byte = -1;
    byte = recv(nms_tcp_recv_client,nms_recv_buff,MAX_FRAME_LEN-1, 0);

    //Disconnect with NMS, try to reconnect
    if(byte == 0){
      printf("client disconnected in recv socket\n");
      if(listen(nms_tcp_recv_server,5) == -1){
        printf("nms_tcp_recv_server listen error!\n");
        exit(0);
      }

      nms_tcp_recv_client = accept(nms_tcp_recv_server, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
      if(nms_tcp_recv_client == -1){
        printf("nms_tcp_recv_server accept error!\n");
        exit(0);
      }
      printf("\nnms_tcp_recv_server accept clear!!!!!! \n");
    }
    else if(byte != -1){
      nmsmsgtype = (NMSCONTROLHDR *)nms_recv_buff;

      printf("[In NMS_TCP_recv_func] : recv %d byte command\n",byte);
      printf("    NMS Command type : %d\n",nmsmsgtype->type);

      switch (nmsmsgtype->type){
        //Case 0 : change metric, recv_ch, data_rate
        case 0:
          nmsmsg = (NMSCONTROL *)nms_recv_buff;

          //metric change
          if(nmsmsg->metric==0 || nmsmsg->metric == metric ){
            printf("    keep current metric\n");
            nmsmsg->metric = metric;
          }
          else if(0<nmsmsg->metric && nmsmsg->metric<4){
            printf("    metric  %u\n\n",nmsmsg->metric);
          }
          else{
            printf("    Wrong metric  %u   ,  so don't change metric \n\n",nmsmsg->metric);
            nmsmsg->metric = metric;
          }

          //data_rate change
          if(nmsmsg->data_rate==0 || nmsmsg->data_rate == data_rate ){
            printf("    keep current data_rate\n");
            nmsmsg->data_rate = data_rate;
          }
          else if(nmsmsg->data_rate == 12 || nmsmsg->data_rate == 18 || nmsmsg->data_rate == 24){
            printf("    data_rate  %u\n",nmsmsg->data_rate);
          }
          else{
            printf("    Wrong data_rate  %u  ,  so don't change datarate \n\n",nmsmsg->data_rate);
              nmsmsg->data_rate = data_rate;
          }

          //recv_ch change
          if(nmsmsg->recv_channel == 0 || nmsmsg->recv_channel == receiving_channel ){
            printf("    keep current recv_channel\n");
            nmsmsg->recv_channel = receiving_channel;
          }
          else if(nmsmsg->recv_channel == 172 || nmsmsg->recv_channel == 174 || nmsmsg->recv_channel == 176 || nmsmsg->recv_channel == 180 || nmsmsg->recv_channel == 182 || nmsmsg->recv_channel == 184){
            printf("    recv_channel  %u\n",nmsmsg->recv_channel);
          }
          else{
            printf("    Wrong recv_channel  %u  ,  so don't change recv_channel \n\n",nmsmsg->recv_channel);
            nmsmsg->recv_channel = receiving_channel;
          }

          //data_rate 변경
          if(data_rate != nmsmsg->data_rate){
            data_rate = nmsmsg->data_rate;
            Reset();
          }
          
          //recv_channel 변경
          if(receiving_channel != nmsmsg->recv_channel){
            receiving_channel = nmsmsg->recv_channel;
            Reset();
          }

          nms_control_msg_process(nms_recv_buff);
        break;

        //Case 1 : on/off about log  (0 : keep / 1 : off / 2 : on)
        case 1:
          if(nmsmsgtype->value == 0){
            printf("    keep current log\n");
            if(nms_log_on==true)
              nmsmsgtype->value=2;
            else 
              nmsmsgtype->value=1;
          }
          nms_control_msg_process(nms_recv_buff);
        break;

        //Case 2 : reset route table (0 : no reset / 1 : reset)
        case 2:
          if(nmsmsgtype->value == 0 || nmsmsgtype->value == 1){
            printf("  reset  %u\n\n",nmsmsgtype->value);
          }
          else{
            printf("  Wrong reset  %u  ,  so fix reset = 0\n\n",nmsmsgtype->value);
            nmsmsgtype->value = 0;
          }
          nms_control_msg_process(nms_recv_buff);
        break;

        //Need to fix!!!!
        //Case 3 : change update cycle (0 : default = 1sec X 1 / 1 : often = 1sec X 0.5 / 2 : less often = 1sec X 1.5)
        case 3:
          printf("  update_cycle  %u\n\n",nmsmsgtype->value);
          if(nmsmsgtype->value == 0){
            update_cycle = 1;
          }
          else if(nmsmsgtype->value==1){
            update_cycle = 1.5;
          }
          else if(nmsmsgtype->value==2){
            update_cycle = 0.5;
          }
          else{
            printf("  Wrong update cycle  %u  ,  so fix default cycle = 1 sec\n\n",nmsmsgtype->value);
            update_cycle = 1;
          }
          nms_control_msg_process(nms_recv_buff);
        break;

        //Case 99 : reboot (reboot addr)
        case 99:
          if(memcmp(&my_mac,&nmsmsgtype->value,6)==0){
            Program_reboot();
          }
          else
            nms_control_msg_process(nms_recv_buff);
        break;

        default:
          printf("  Wrong NMS Command type(0~4) : %d \n",nmsmsgtype->type);
        break;
      }
    }
  }
}


void* Command_thread_func(void)
{
  static int choiceNum = -1;
  int re = 0, index = -1;
  uint8_t val[6]={0};
  //struct rt_table *rt;
  uint8_t rootinit[6] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

  char buff[1024] ="";
  int i = 0;

  char buf[30];
  NMSCONTROL *nmsmsg;
  nmsmsg = (NMSCONTROL *)buf;

  while(main_state)
  {
    sleep(5);
      //printf("check config_end %d\n", config_end);
    
    // if(config_end)
    if(1)
    {
	choiceNum = -1;
	printf("Please select menu \n");
	printf("< mesh >-------------------------------\n");		
	printf(" [1] Start config again\n");
	printf(" [2] PREQ/PREP\n");
	printf(" [3] Hello Timeout start(every 20sec/during 10sec)\n");
	printf(" [4] Delete rt_table\n");
	printf(" [5] Insert New Root Address\n");
	printf(" [6] Print rt_table & NMS_table\n");
	printf(" [7] Data Send to Nexthop\n");
  printf(" [8] Reset\n");
  printf(" [9] Recv channel change\n");
  printf(" [10] Data rate change\n");
  printf(" [11] metric change\n");
  printf(" [13] PREP send in cch \n");
  printf(" [14] PREQ send in sch \n");
  printf(" [15] Data send in cch \n");
  printf(" [19] Root turn off by nmsmsg! \n");

	printf(" [0] Exit\n");	

	printf("Enter your choice : \n");

        re = scanf("%d", &choiceNum);
        printf("%s\n\n", __bar);  
   
    	switch (choiceNum)
    	{
	   case 1:
		//printf("Config time(Default 90 sec) : \n");
		//re = scanf("%d", &val);
		//(void) re;

		//if(val >= 20 && val <= 300)
			//config_time = val/2;
		Timer_Reset();
		rt_table_empty(10);
		rann_order = 0;

		Hello_start();

		if(RootNode == 1) {
			printf("\n ***** I AM ROOT NODE *****\n\n");		
			// Rann_start();		
		}

		else printf("\n ----- I AM I2I MESH NODE -----\n\n");

		// alm_start();	

		break;	

  	  case 2:
		
		re = -99;
		//rt = rt_table_find(root_addr);
		re = memcmp(&root_addr.addr, &rootinit, LL_ADDR_SIZE);
		rann_order ++;		
		//if ((!rt) && (RootNode != 1) && (re != 0) ) 
		if ((RootNode != 1) && (re != 0) ) 
		{	
			// Preq_send(&root_addr,rann_order,receiving_channel);
		} 
		else if (RootNode == 1) //will be send PREQ to another 2-hop neighbor node.
			printf(" ---- I am ROOT, No need PREQ\n");
		else
		 	printf(" ---- Already have ROOT node routing\n");
		re = 0;
		break;
	
	   case 3:
		Timer_Reset();
		Hello_timeout_start();
		break;

 	   case 4:

		 printf("index(0~9, all:10) ==> ");
		 re = scanf("%d", &index);

		 if(index < 0 || index > 10)
			printf("index invalid (0~9)\n");
	
		else
		{
			rt_table_empty(index);
			printf("Delete rt_table index %d\n", index);
		}
		break;

	   case 5:

		printf("input(00:00:00:00:00:00) ==> ");
		re = scanf("%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
		if(re != 6)
			printf("address format invalid\n");
		else {
			memcpy(&root_addr, &val,6);
			printf("New root addr: %02x:%02x:%02x:%02x:%02x:%02x\n", root_addr.addr[0],  root_addr.addr[1],  root_addr.addr[2],  root_addr.addr[3],  root_addr.addr[4],  root_addr.addr[5]);
		}

		break;
		
	  case 6:
		  rt_table_print();
      nms_table_print();
		break;

 	  case 7:
      print_time = 1; 
      Init_Q();
      for(i = 0; i < 1000; i ++)
        buff[i] = i;
      Data_start2(&p_rt_list->rt_t[0].dest_addr, &buff[0], 1000);
		break;

    case 8:
		  Reset();
		break;

    case 9:
    if(RootNode==1){
      nmsmsg->type = 0;
      nmsmsg->metric = 1;
      nmsmsg->data_rate=data_rate;
      nmsmsg->recv_channel=174;     
      receiving_channel = nmsmsg->recv_channel;
      Reset();
      nms_control_msg_process(buf);
    }
    break;

    case 10:
    if(RootNode==1){
      nmsmsg->type = 0;
      nmsmsg->metric = 1;
      nmsmsg->data_rate=18;
      nmsmsg->recv_channel=receiving_channel;     
      data_rate = nmsmsg->data_rate;
      Reset();
      nms_control_msg_process(buf);
    }
    break;

    case 11:
    if(RootNode==1){
      nmsmsg->type = 0;
      nmsmsg->metric = 2;
      nmsmsg->data_rate=data_rate;
      nmsmsg->recv_channel=receiving_channel;     
      metric = nmsmsg->metric;
      nms_control_msg_process(buf);
    }
    break;


    case 13:
    if(RootNode==1){
		PREP_TEST_CCH(&p_rt_list->rt_t[0].dest_addr);
    }
    break;

    case 14:
    if(RootNode==1){
		PREP_TEST_SCH(&p_rt_list->rt_t[0].dest_addr);
    }
    break;

    case 15:
    if(RootNode==1){
				print_time = 1; 
		Init_Q();

		for(i = 0; i < 1000; i ++)
			buff[i] = i;

		Data_start3(&p_rt_list->rt_t[0].dest_addr, &buff[0], 1000);

    }
    break;

    case 19:
    if(RootNode==1){
      Program_reboot();
    }
    break;

	   case 0:
		main_state = false;
		Timer_Reset();
		break;

	  default:
		break;

	}// switch
    }//if


  }//while

  printf("[CommandThread Bye]\n");
  return 0;
}


void* Netlink_thread_func(void)
{
	unsigned int *p0;
	unsigned int size;
	int receive_size;
	// char *pbuf;
	//struct iphdr *ip;

	printf("In Netlink Thread!!\n");
	
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); // self pid
	
	bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; // For Linux Kernel
	dest_addr.nl_groups = 0; // unicast

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	strcpy(NLMSG_DATA(nlh), "Hello! I'm user");

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	//int i = 0;

	printf("Sending message to kernel\n");
	sendmsg(sock_fd,&msg,0);
	printf("Waiting for message from kernel\n");

	while(1)
	{
		
		memset(NLMSG_DATA(nlh),0,MAX_PAYLOAD);
		receive_size=recvmsg(sock_fd, &msg, 0);
		if(receive_size<28 || receive_size>4096)
			continue;

		p0 = (unsigned int *)NLMSG_DATA(nlh);
		memcpy(&size,&p0[0],4);

		//ip = (struct iphdr *) &p0[1];
		//p1 = &(ip->saddr); 
		//p2 = &(ip->daddr);


		//printf("Receive Netlink msg : [Src] %d.%d.%d.%d -> [Dst] %d.%d.%d.%d \n", p0[4]&0xFF, (p0[4]>>8)&0xFF, (p0[4]>>16)&0xFF,(p0[4]>>24)&0xFF, p0[5]&0xFF, (p0[5]>>8)&0xFF, (p0[5]>>16)&0xFF,(p0[5]>>24)&0xFF);
		
		//printf("Receive Netlink msg : [Src] %d.%d.%d.%d -> [Dst] %d.%d.%d.%d \n",p1[3],p1[2], p1[1], p1[0], p2[3], p2[2], p2[1], p2[0]);

		memcpy(&pIPH->SrcIP,&p0[4],4);
		memcpy(&pIPH->DstIP,&p0[5],4);

		if ( (pIPH->SrcIP != pIPH->DstIP) && ((p0[5]>>24)&0xFF) != 255 ) 
		{
			// printf("Receive Netlink msg : [Src] %d.%d.%d.%d -> [Dst] %d.%d.%d.%d \n", p0[4]&0xFF, (p0[4]>>8)&0xFF, (p0[4]>>16)&0xFF,(p0[4]>>24)&0xFF, p0[5]&0xFF, (p0[5]>>8)&0xFF, (p0[5]>>16)&0xFF,(p0[5]>>24)&0xFF);
		

			// printf("                      [received] : %d ( NLM Header : 16 ) \n",receive_size);
	
			// printf("	              [size] : %d ( IP Header(20) + TCP/UDP(8) Header + Data ) \n",size);
			// pbuf=(char*)&p0[1];

			// /*for (i = 0; i < (int) size; i ++)
			// {
			// 	if( i > 0 && i % 16 == 0) printf("\n");
			// 	printf(" %02x", pbuf[i]);
			// }

			// printf("\n"); */
			// Data_start(pbuf,size);
		}
	}

        printf("[Netlink_thread_func Bye]\n");
	return 0;
}

void* Hello_Rann_thread_func(void)
{
	printf("In Hello_Rann_thread!!\n");
	printf("Hello & Rann start!\n");

	struct timeval now;

	Hello_start();
	Hello_timeout_start();	
  SCH_Hello_start();
  alm_start();
  API_reset_timer_start();

	if(RootNode == 1) {
		printf("\n ***** I AM ROOT NODE *****\n\n");
		Rann_start();		
	}

	else printf("\n ----- I AM I2I MESH NODE -----\n\n");

	while(main_state)
	{
		gettimeofday(&now, NULL);
		Timer_timeout(&now);
	}

 	printf("[Hello_Rann_thread_func Bye]\n");
	
	return 0;
}


/**
 * @brief 어플리케이션 메인 함수
 * @param[in] argc 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 개수 (유틸리티 실행파일명 포함)
 * @param[in] argv 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 문자열 집합 (유틸리티 실행파일명 포함)
 * @retval 0: 성공
 * @retval -1: 실패
 */
int main(int argc, char *argv[])
{
  // Step 1. 한컴 장비 설정------------------------------------------------------------------------------------------------------------------------------
  /*
  * 아무 파라미터 없이 실행하면 사용법을 출력한다.
  */
  if (argc < 2) {
    WSM_Usage(argv[0]);
    return 0;
  }

  printf("Running WSM trx application..\n");

  /*
  * 입력 파라미터를 파싱하여 저장한다.
  */
  memset(&g_mib, 0, sizeof(g_mib));
  int ret = WSM_ParsingInputParameters(argc, argv);
  if (ret < 0) {
    return -1;
  }

  /*
  * V2X 라이브러리들을 초기화한다.
  */
  ret = WSM_InitV2XLibs();
  if (ret < 0) {
    return -1;
  }

  /*
  * Ctrl + C, Kill 신호에 대한 핸들러를 등록한다 - 종료 시 WAL_Close()를 호출하기 위함이다.
  */
  if (signal(SIGINT, WSM_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGINT) - %m\n");
    ret = -1;
    goto out;
  }
  if (signal(SIGTERM, WSM_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGTERM) - %m\n");
    ret = -1;
    goto out;
  }

  /*
  * 각 인터페이스의 MAC 주소를 확인한다 (WSM MPDU의 MAC 헤더에 수납하기 위해)
  */
  for (unsigned int if_idx = 0; if_idx < g_mib.if_num; if_idx++) {
    ret = WAL_GetIfMACAddress(if_idx, g_mib.my_addr[if_idx]);
    if (ret < 0) {
      printf("Fail to WAL_GetIfMACAddress() for if[%u] - %d\n", if_idx, ret);
      goto out;
    }
    printf("Check if[%u] MAC address - "MAC_ADDR_FMT"\n", if_idx, MAC_ADDR_FMT_ARGS(g_mib.my_addr[if_idx]));
  }

  /*
  * set channel number using user specified options
  */
  ret = WAL_AccessChannel(g_mib.tx_if_idx, g_mib.tx_chan_num, g_mib.tx_chan_num);//Default : interface 0-178-178(Continuous)

  if (ret < 0)
  {
      printf("Channel number configuration is failed(error=%d) for if=%d ch=%d", ret, g_mib.tx_if_idx, g_mib.tx_chan_num);
      goto out;
  }

  ret = WAL_AccessChannel(DEFAULT_IF_IDX+1, g_mib.tx_chan_num2, g_mib.tx_chan_num2);//RF2 : interface 1-172-172(Continuous)

  if (ret < 0)
  {
      printf("Channel number configuration is failed(error=%d) for if=%d ch=%d/%d", ret, g_mib.tx_if_idx+1, g_mib.tx_chan_num2, g_mib.tx_chan_num2);
      goto out;
  }

  /*
  * WSM 수신을 위한 PSID를 등록한다.
  */
  ret = Dot3_AddWSR(g_mib.psid);
  if (ret < 0) {
    printf("Fail to add WSR(psid: %u) - %d\n", g_mib.psid, ret);
    goto out;
  }

  /*
  * 메시지 송신 루틴을 초기화한다.
  */
  if (g_mib.op == kOperationType_trx) {
    ret = WSM_InitTxOperation(g_mib.tx_interval);
    if (ret < 0) {
      goto out;
    }
  }


  receiving_channel = 172;
  data_rate = g_mib.tx_datarate;

// Step 1 Finish----------------------------------------------------------------------------------------------------------------------------------------


// Step 2. Ajou setting---------------------------------------------------------------------------------------------------------------------------------

//NMS log unicast init -> each log가 모두 NMS로 전달 
NMSLOGINFO nms_log_str;
memset(&nms_log_str, 0, NMSLOGINFO_SIZE);
p_nms_log_str = &nms_log_str;

memcpy(&p_nms_log_str->orig_addr,&my_mac,6);
p_nms_log_str->type = 6;
p_nms_log_str->size = 0;

//NMS gateway info init -> routing protocol을 사용하여 Root 가 모을수 있는 정보를 모아서 전달 
// if(RootNode == 1){
  struct nms_table_list nms_list;
  p_nms_list = &nms_list;
  memset(&nms_list, 0, NMS_TABLE_SIZE);
// }

//Routing table init
  struct rt_table_list rt_list;
  p_rt_list = &rt_list;

  rt_table_init(p_rt_list);

  int a = 1;
  int status = 0;

//Netlink set
  sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
 
  if(sock_fd<0)
  {
	printf("Netlink Socket open failed!\nPlease check that kajou module is running\n");
  }
  else
  {
	thr_rc = pthread_create(&pNetlinkThread,NULL,(void*) Netlink_thread_func,(void *)&a);	

	if(thr_rc < 0)
	{
		printf("Netlink thread create error!!\n");
		exit(0);
	}
  }

//Hello_Rann Message & Command Thread
  thr_rc = pthread_create(&pCommandThread,NULL,(void*) Command_thread_func,(void *)&a);
  if(thr_rc < 0)
  {
	printf("Command thread create error!!\n");
	exit(0);
  }

  sleep(1);
  thr_rc = pthread_create(&pHelloRannThread,NULL,(void*) Hello_Rann_thread_func,(void *)&a);
  if(thr_rc < 0)
  {
	printf("Hello&Rann thread create error!!\n");
	exit(0);
  }

  if(RootNode == 1){
    thr_rc = pthread_create(&pNMS_send_Thread,NULL,(void*) NMS_TCP_send_func, (void *)&a);
    if(thr_rc < 0){
      printf("NMS TCP comm func thread create error!!\n");
      exit(0);
    }
    thr_rc = pthread_create(&pNMS_recv_Thread,NULL,(void*) NMS_TCP_recv_func, (void *)&a);
    if(thr_rc < 0){
      printf("NMS TCP comm func thread create error!!\n");
      exit(0);
    }
  }


  /*
   * 무한 루프
   *  - (송신 동작 시) WSM 송신 타이머 처리
   *  - WSM 수신 콜백 처리 alm_start
   */
  while (main_state) {
    usleep(1000000);
    fflush(stdout);
  }

//Netlink socket_fd error
  if(sock_fd >= 0)
  {
	  shutdown(sock_fd, SHUT_WR);
  	close(sock_fd);
	  pthread_detach(pNetlinkThread);
  }
  
  pthread_join(pCommandThread,(void **)&status); 
  pthread_join(pHelloRannThread,(void **)&status);
  if(RootNode == 1){
    pthread_join(pNMS_send_Thread, (void **)&status);
    pthread_join(pNMS_recv_Thread, (void **)&status);
  }

  printf("\n[MAIN] BYE BYE~\n");
 
  ret = 0;

out:
  Dot3_Release();
  WAL_Close();
  if(RootNode==1){
    close(nms_tcp_send_server);
    close(nms_tcp_send_client);
    close(nms_tcp_recv_server);
    close(nms_tcp_recv_client);
  }

  return ret;
}



/**
 * @brief 로그 메시지를 출력한다.
 * @param[in] func 본 함수를 호출하는 함수의 이름
 * @param[in] format 출력 메시지 인자
 * @param[in] ... 출력 메시지 인자
 */
void WSM_Print(const char *func, const char *format, ...)
{
  va_list arg;
  struct timespec ts;
  struct tm tm_now;

  clock_gettime(CLOCK_REALTIME, &ts);
  localtime_r((time_t *)&ts.tv_sec, &tm_now);
  fprintf(stderr, "[%04u%02u%02u.%02u%02u%02u.%06ld][%s] ", tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
          tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, ts.tv_nsec / 1000, func);
  va_start(arg, format);
  vprintf(format, arg);
  va_end(arg);
}
void WSM_Print2(const char *func)
{
  struct timespec ts;
  struct tm tm_now;

  clock_gettime(CLOCK_REALTIME, &ts);
  localtime_r((time_t *)&ts.tv_sec, &tm_now);
  fprintf(stderr, "[%04u%02u%02u.%02u%02u%02u.%06ld][%s] ", tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
          tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, ts.tv_nsec / 1000, func);  
}

