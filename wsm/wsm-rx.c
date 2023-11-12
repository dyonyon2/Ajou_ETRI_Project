/**
 * @file
 * @brief WSM 수신 처리 기능을 구현한 파일
 * @date 2019-08-12
 * @author gyun
 */


// 시스템 헤더 파일
#include <stdio.h>

// 라이브러리 헤더 파일
#include "v2x-sw.h"

// 어플리케이션 헤더 파일
#include "wsm.h"
#include "aodv.h"
#include "mesh_extern.h"

/**
 * @brief 수신된 WSDU를 처리한다.
 * @param[in] parse_info 패킷파싱데이터
 *
 * 샘플 어플리케이션이므로 단순히 정보 출력만 수행한다.
 */
void WSM_ProcessRxWSDU(const struct V2XPacketParseData *parse_info)
{
  // printf("In recv event!!!!!\n");
  if (g_mib.dbg >= kDbgMsgLevel_event)
  {
    /*
     * 관심 있는 서비스에 대한 WSDU를 처리한다.
     */
    if (parse_info->interested_psid == true)
    {
      // WSM_Print(__FUNCTION__, "Process interested rx WSDU\n");

      // 수신 파라미터 정보
      // const struct WalMPDURxParams *rx_params = &(parse_info->rx_params);

      // WSM_Print(__FUNCTION__,
      //           "    Rx info - if: %u, timeslot: %u, chan: %u, rx power: %ddBm, rcpi: %u, datarate: %u*500kbps\n",
      //           rx_params->if_idx, rx_params->timeslot, rx_params->chan_num,
      //           rx_params->rx_power, rx_params->rcpi, rx_params->datarate);
      
      // MAC 및 WSM 헤더 정보
      const struct Dot3MACAndWSMParseParams *mac_wsm = &(parse_info->mac_wsm);
      /*WSM_Print(__FUNCTION__,
                "    MAC header - dst: "MAC_ADDR_FMT", src: "MAC_ADDR_FMT", priority: %u\n",
                MAC_ADDR_FMT_ARGS(mac_wsm->mac.dst_mac_addr), MAC_ADDR_FMT_ARGS(mac_wsm->mac.src_mac_addr),
                mac_wsm->mac.priority);
      WSM_Print(__FUNCTION__,
                "    WSM header - PSID: %u, chan: %u, datarate: %u*500kbps, power: %ddBm\n",
                mac_wsm->wsm.psid, mac_wsm->wsm.chan_num, mac_wsm->wsm.datarate, mac_wsm->wsm.transmit_power);

      // 각 데이터 유닛 길이
      WSM_Print(__FUNCTION__,
                "    Size - MPDU: %u, WSM: %u, WSDU(Payload): %u\n",
                parse_info->mpdu_size, parse_info->wsm_size, parse_info->wsdu_size);*/
 
      uint8_t outbuf[parse_info->wsm_size];

     memcpy(&outbuf, parse_info->wsdu,parse_info->wsm_size ); 
     //2020.07.30 ETRI ADD for Receive Hello/rann Protocol.
     memcpy(&src_mac_addr, mac_wsm->mac.src_mac_addr,6);

   
     if(print_time == 1)
		WSM_Print2(__FUNCTION__);

     Receive_process(outbuf);  


      // WSDU
      if (g_mib.dbg >= kDbgMsgLevel_msgdump) {
        for (size_t i = 0; i < parse_info->wsdu_size; i++) {
          if ((i != 0) && (i % 16 == 0)) { printf("\n"); }
          printf("%02X ", parse_info->wsdu[i]);
        }
        printf("\n");
      }
    }

    /*
     * 관심 없는 서비스에 대한 WSDU는 처리하지 않는다.
     */
    else
    {
      WSM_Print(__FUNCTION__, "NOT process WSM - not intersted PSID %u\n", parse_info->mac_wsm.wsm.psid);
    }
  }
}


/**
 * @brief MPDU 수신처리 콜백함수. 접속계층 라이브러리에서 호출된다.
 * @param[in] mpdu 수신된 MPDU
 * @param[in] mpdu_size 수신된 MPDU의 크기
 * @param[in] rx_params 수신 파라미터 정보
 */
void WSM_ProcessRxMPDUCallback(const uint8_t *mpdu, WalMPDUSize mpdu_size, const struct WalMPDURxParams *rx_params)
{
  if (g_mib.dbg >= kDbgMsgLevel_event) {
    //WSM_Print(__FUNCTION__, "Process rx MPDU\n");
  }

  /*
   * 패킷파싱데이터를 할당한다.
   */
  struct V2XPacketParseData *parsed = V2X_AllocatePacketParseData(mpdu, mpdu_size, rx_params);
  if (parsed == NULL) {
    WSM_Print(__FUNCTION__, "Fail to process rx MPDU - V2X_AllocatePacketParseData() failed\n");
    return;
  }

  /*
   * WSM MPDU를 파싱한다.
   */
  int ret;
  parsed->wsdu = Dot3_ParseWSMMPDU(parsed->mpdu,
                                       parsed->mpdu_size,
                                       &(parsed->mac_wsm),
                                       &(parsed->wsdu_size),
                                       &(parsed->interested_psid),
                                       &ret);

  if (parsed->wsdu == NULL) {
    WSM_Print(__FUNCTION__, "Fail to process rx MPDU - Dot3_ParseWSMMPDU() failed: %d\n", ret);
    V2X_FreePacketParseData(parsed);
    return;
  }

  /*
   * WSDU를 처리한다.
   */
  WSM_ProcessRxWSDU(parsed);

  /*
   * 패킷파싱데이터를 해제한다.
   */
  V2X_FreePacketParseData(parsed);

#ifdef _MSG_PROCESS_LATENCY_TEST_
  WSM_Print(__FUNCTION__, "WSM received\n");
#endif
}