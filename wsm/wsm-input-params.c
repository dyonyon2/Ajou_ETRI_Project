/**
 * @file
 * @brief wsm 어플리케이션 실행 시 함께 입력되는 파라미터들을 처리하는 기능을 구현한 파일
 * @date 2019-08-10
 * @author gyun
 */

// 시스템 헤더 파일
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 라이브러리 헤더 파일
#include "v2x-sw.h"

// 어플리케이션 헤더 파일
#include "wsm.h"
//add by ETRI, neuron. 2020.10.30
#include "mesh_extern.h"
#include "aodv.h"

/**
 * @brief 옵션식별값에 따라 각 옵션을 처리한다.
 * @param[in] option_idx 옵션식별값 (struct option 의 4번째 멤버변수)
 */
static int ip_set = 0;

static void WSM_ProcessParsedOption(int option_idx)
{
// add by ETRI, neuron. 2020.10.30 
  uint32_t temp_val = 0, val = 0;
  uint8_t connect_ip[4] ;  // option --ip 
  switch (option_idx) {
    case 0: {
      g_mib.tx_if_idx = (unsigned int)strtoul(optarg, 0, 10);
      break;
    }
    case 1: {
      g_mib.psid = (Dot3PSID)strtol(optarg, 0, 10);
      break;
    }
    case 2: {
      g_mib.tx_chan_num = (Dot3ChannelNumber)strtol(optarg, 0, 10);
      break;
    }
    case 3: {
      g_mib.tx_datarate = (Dot3DataRate)strtoul(optarg, 0, 10);
      break;
    }
    case 4: {
      g_mib.tx_power = (Dot3Power)strtol(optarg, 0, 10);
      break;
    }
    case 5: {
      g_mib.tx_priority = (Dot3Priority)strtoul(optarg, 0, 10);
      break;
    }
    case 6: {
      WAL_ConvertMACAddressStrToOctets(optarg, g_mib.tx_dst_mac_addr);
      break;
    }
    case 7: {
      g_mib.tx_wsm_body_len = (Dot3WSMPayloadSize)strtoul(optarg, 0, 10);
      break;
    }
    case 8: {
      g_mib.tx_interval = (unsigned int)strtoul(optarg, 0, 10);
      break;
    }
    case 9: {
      g_mib.dbg = strtoul(optarg, 0, 10);
      break;
    }
    case 10: //for Mesh Root Node
      temp_val = 0;
	  temp_val = strtoul(optarg, 0, 10);

      /* sanity check */
     if (temp_val < 2)
      {
              RootNode_Set(temp_val);
      }
      else    RootNode_Set(DEFAULT_ROOT_VALUE);      
      break;

   case 11: //my_ip
      ip_set = 1;

      temp_val = 0;
      sscanf(optarg, "%hhd.%hhd.%hhd.%hhd", &connect_ip[0], &connect_ip[1], &connect_ip[2],  &connect_ip[3]);
      temp_val = (connect_ip[3] << 24)+ (connect_ip[2] << 16) + (connect_ip[1] << 8) + connect_ip[0];
      printf("ip - %d.%d.%d.%d\n", connect_ip[0], connect_ip[1], connect_ip[2], connect_ip[3]);

      IP_Set(temp_val);

      break;

   case 12: //For Mesh Config Metric 1, 2
      temp_val = strtoul(optarg, 0, 10);
      /* sanity check */
      if (temp_val < 2)
      {
              metric = temp_val;
      }
      else    metric = DEFAULT_METRIC_VALUE;

      printf("metric select(0:hop, 1:alm) - %d\n", metric);
      break;

    case 13: 
      temp_val = 0;
      temp_val = strtoul(optarg, 0, 10);
      connect_ip[0] = 192;
      connect_ip[1] = 168;
      connect_ip[2] = 20;

      if(temp_val == 0) {  
        printf("in case 13 temp_val\n\n\n");
        connect_ip[3] = 12;
        RootNode_Set(1);
      }
      else if(temp_val == 1)   
	      connect_ip[3] = 13;
      else if(temp_val == 2)   
	      connect_ip[3] = 14;
      else 
        connect_ip[3] = 15;

      if(ip_set == 0)
      {
      	printf("hop %d ip -  %d.%d.%d.%d\n", temp_val, connect_ip[0], connect_ip[1], connect_ip[2], connect_ip[3]);
      val = (connect_ip[3] << 24)+ (connect_ip[2] << 16) + (connect_ip[1] << 8) + connect_ip[0];
      	IP_Set(val);
      }
      break;
    
    case 14:
      temp_val = 0;
      temp_val = strtoul(optarg, 0, 10);
      if(temp_val == 1)
        nms_log_on = true;
      else 
        nms_log_on = false;

      printf(nms_log_on?"nms_log on\n":"nms_log off\n");
      break;

    default: {
      break;
    }
  }
}


/**
 * @brief 어플리케이션 실행 시 함께 입력된 파라미터들을 파싱하여 관리정보에 저장한다.
 * @param[in] argc @ref V2X_WSM_Main
 * @param[in] argv @ref V2X_WSM_Main
 * @retval 0: 성공
 * @retval -1: 실패
 */
int WSM_ParsingInputParameters(int argc, char *argv[])
{
  int c, option_idx = 0;
  struct option options[] = {
    {"if", required_argument, 0, 0/*=getopt_long() 호출 시 option_idx 에 반환되는 값*/},
    {"psid", required_argument, 0, 1},
    {"chan", required_argument, 0, 2},
    {"rate", required_argument, 0, 3},
    {"power", required_argument, 0, 4},
    {"prio", required_argument, 0, 5},
    {"dst", required_argument, 0, 6},
    {"len", required_argument, 0, 7},
    {"interval", required_argument, 0, 8},
    {"dbg", required_argument, 0, 9},
    {"root",required_argument, 0, 10}, 
    {"ip", required_argument, 0, 11},  
    {"metric", required_argument, 0, 12},
    {"hop", required_argument, 0, 13},
    {"log", required_argument, 0, 14},
    {0, 0, 0, 0} // 옵션 배열은 {0,0,0,0} 센티넬에 의해 만료된다.
  };

  /*
   * 기본 파라미터 파싱 및 저장
   */
  #if 0
  if (!memcmp(argv[1], "trx", 3)) {
    g_mib.op = kOperationType_trx;
  } else if (!memcmp(argv[1], "rx", 2)) {
    g_mib.op = kOperationType_rx_only;
  } else {
    printf("Invalid operation - %s\n", argv[1]);
    return -1;
  }

  #endif

  g_mib.op = kOperationType_trx;

  /*
   * 파라미터 기본 값 설정
   */
  g_mib.tx_if_idx = DEFAULT_IF_IDX;
  g_mib.psid = DEFAULT_PSID;
  g_mib.tx_chan_num = DEFAULT_CHAN_NUM;

  g_mib.tx_chan_num2 = DEFAULT_CHAN_NUM2;
  // g_mib.tx_chan_num3 = DEFAULT_CHAN_NUM3;

  g_mib.tx_datarate = DEFAULT_DATARATE;
  g_mib.tx_power = DEFAULT_POWER;
  g_mib.tx_priority = DEFAULT_PRIORITY;
  g_mib.tx_wsm_body_len = DEFAULT_WSM_BODY_LEN;
  g_mib.tx_interval = DEFAULT_TX_INTERVAL;
  g_mib.dbg = DEFAULT_DBG;
  memset(g_mib.tx_dst_mac_addr, 0xff, MAC_ALEN);

  /*
   * 파라미터 파싱 및 저장
   */
  while(1) {

    /*
     * 옵션 파싱
     */
    c = getopt_long(argc, argv, "", options, &option_idx);
    if (c == -1) {  // 모든 파라미터 파싱 완료
      break;
    }

    /*
     * 파싱된 옵션 처리
     */
    WSM_ProcessParsedOption(option_idx);
  }

#ifdef _MSG_PROCESS_LATENCY_TEST_
  g_mib.dbg = kDbgMsgLevel_nothing;
#endif

  /*
   * 파싱된 파라미터 내용 출력
   */
  printf("  op: %u(0:rx,1:trx), if_num: %u, psid: %u, dbg: %u\n", g_mib.op, g_mib.if_num, g_mib.psid, g_mib.dbg);
  if (g_mib.op == kOperationType_trx) {
    printf("  if_idx: %u, chan: %u\n", g_mib.tx_if_idx, g_mib.tx_chan_num);
    printf("  datarate: %u*500kbps, power: %ddBm, priority: %u, dst: "MAC_ADDR_FMT"\n",
           g_mib.tx_datarate, g_mib.tx_power, g_mib.tx_priority, MAC_ADDR_FMT_ARGS(g_mib.tx_dst_mac_addr));
    printf("  body len: %zu, interval: %u(usec)\n", g_mib.tx_wsm_body_len, g_mib.tx_interval);
  }

  return 0;
}
