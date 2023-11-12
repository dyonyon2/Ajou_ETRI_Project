/**
 * @file
 * @brief 유틸리티에서 wlanaccess 라이브러리를 사용하는 기능을 구현한 파일
 * @date 2019-08-15
 * @author gyun
 */


// 시스템 헤더 파일
#include <stdio.h>

// 라이브러리 헤더 파일
#include "wlanaccess/wlanaccess.h"

// 유틸리티 헤더 파일
#include "chan.h"


/**
 * @brief 접속계층라이브러리를 오픈한다.
 *
 * @param[in] log_level 라이브러리 로그메시지 출력레벨
 * @retval 0: 성공
 * @retval -1: 실패
 */
int CHAN_OpenAccessLibrary(WalLogLevel log_level)
{
  /*
   * 라이브러리를 오픈한다.
   */
  int ret = WAL_Open(log_level);
  if (ret < 0) {
    printf("Fail to open access library - WAL_Open() failed: %d\n", ret);
    return -1;
  }
  g_if_num = ret;
  printf("Success to open access library - %d interface supported\n", ret);
  return 0;
}


/**
 * @brief 채널 접속을 수행한다.
 *
 * @param[in] if_idx 채널접속할 인터페이스 식별번호
 * @param[in] ts0_chan_num TS0 에 접속할 채널번호
 * @param[in] ts1_chan_num TS1 에 접속할 채널번호
 * @retval 0: 성공
 * @retval -1: 실패
 */
int CHAN_AccessChannel(uint8_t if_idx, WalChannelNumber ts0_chan_num, WalChannelNumber ts1_chan_num)
{
  int ret = WAL_AccessChannel(if_idx, ts0_chan_num, ts1_chan_num);
  if (ret < 0) {
    printf("Fail to access channel %d-%d for if[%u]- WAL_AccessChannel() failed: %d\n",
      ts0_chan_num, ts1_chan_num, if_idx, ret);
    return ret;
  }
  printf("Success to access channel for if[%u] : %d-%d\n", if_idx, ts0_chan_num, ts1_chan_num);
  return 0;
}


/**
 * @brief 모든 V2X 통신인터페이스의 접속채널을 화면에 출력한다.
 * @retval 0: 성공
 * @retval -1: 실패
 */
int CHAN_PrintCurrentChannel(void)
{
  int ret;
  WalChannelNumber ts0_chan_num, ts1_chan_num;
  for (int if_idx = 0; if_idx < g_if_num; if_idx++) {
    ret = WAL_GetCurrentChannel(if_idx, &ts0_chan_num, &ts1_chan_num);
    if (ret < 0) {
      printf("Fail to get current channel for if[%u] - WAL_GetCurrentChannel() failed: %d\n", if_idx, ret);
      return ret;
    }
    printf("Success to get current channel for if[%u] : %d-%d\n", if_idx, ts0_chan_num, ts1_chan_num);
  }
  return 0;
}
