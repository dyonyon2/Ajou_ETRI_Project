/**
 * @file
 * @brief chan 유틸리티 구현 메인 파일
 *
 * 본 유틸리티는 V2X 통신인터페이스의 채널 접속, 확인, 초기화 등의 기능을 수행한다. \n
 * 본 유틸리티는 접속계층라이브러리 API를 사용한다.
 */


// 시스템 헤더 파일
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 라이브러리 헤더 파일
#include "wlanaccess/wlanaccess.h"

// 유틸리티 헤더 파일
#include "chan.h"


/*
 * 입력 파라미터 전역변수
 */
Operation g_op; ///< 어플리케이션 동작 유형
uint8_t g_if_num; ///< v2x 인터페이스 총 개수 (플랫폼 하드웨어에 의존적이다)
uint8_t g_if_idx; ///< v2x 인터페이스 식별번호
WalChannelNumber g_chan_num[2]; ///< TS0, TS1 채널
bool g_loop = false; ///< 유틸리티가 종료되지 않고 계속 실행되도록 설정하는 변수


/**
 * @brief 유틸리티 사용법을 화면에 출력한다.
 * @param[in] app_filename 유틸리티 실행파일명
 */
static void CHAN_Usage(char app_filename[])
{
  printf("\n\n Description: Utility to check and configure the channel using v2x-sw libraries\n");

  printf("\n Usage: %s check|config [OPTIONS]\n\n", app_filename);
  printf("          check:  checking current channel of all interfaces\n");
  printf("          config: setting channel of specific interface\n");

  printf("\n OPTIONS for \"CHECK\" operation\n");
  printf("  --loop                Set the utility not to end. To kill, use Ctrl+C\n");

  printf("\n OPTIONS for \"CONFIG\" operation\n");
  printf("  --if <if_idx>         Set interface index(0~n). If not specified, set to %d\n", DEFAULT_IF_IDX);
  printf("  --ts0 <chan_num>      Set channel for timeslot0. If not specified, set to %d\n", DEFAULT_CHAN_NUM);
  printf("  --ts1 <chan_num>      Set channel for timeslot1. If not specified, set to %d\n", DEFAULT_CHAN_NUM);
  printf("  --loop                Set the utility not to end. To kill, use Ctrl+C\n");

  printf("\n Further description\n");
  printf("  * If same channel is specified for ts0 and ts1, \"CONTINUOUS\" access is performed\n");
  printf("  * If different channel is specified for ts0 and ts1, \"ALTERNATING\" access is performed\n");

  printf("\n Example\n");
  printf("  1) %s check                         Checking current channels\n", app_filename);
  printf("  2) %s config --ts0 184 --ts1 184    Setting interface 0 channel with 184 continuous access\n", app_filename);
  printf("  3) %s config --ts0 172 --ts1 184    Setting interface 1 channel with 172-184 alternating access\n", app_filename);
  printf("\n\n");
}


/**
 * @brief 종료신호 핸들러. WAL_Close()를 호출한다.
 * @param signo 사용되지 않음.
 */
static void CHAN_SigHandler(int signo)
{
  (void)signo;
  printf("Program exits...\n");
  WAL_Close();
  exit(0);
}


/**
 * @brief 유틸리티 메인 함수
 *
 * @param[in] argc 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 개수 (유틸리티 실행파일명 포함)
 * @param[in] argv 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 문자열 집합 (유틸리티 실행파일명 포함)
 * @retval 0: 성공
 * @retval -1: 실패
 */
int main(int argc, char *argv[])
{
  /*
   * 아무 파라미터 없이 실행하면 사용법을 출력한다.
   */
  if (argc < 2) {
    CHAN_Usage(argv[0]);
    return 0;
  }

  printf("Running chan utility..\n");

  /*
   * 입력 파라미터를 파싱하여 저장한다.
   */
  int ret = CHAN_ParsingInputParameters(argc, argv);
  if (ret < 0) {
    return -1;
  }

  /*
   * 접속계층라이브러리를 오픈한다.
   */
  ret = CHAN_OpenAccessLibrary(kWalLogLevel_Err);
  if (ret < 0) {
    return -1;
  }

  /*
   * Ctrl + C, Kill 신호에 대한 핸들러를 등록한다 - 종료 시 WAL_Close()를 호출하기 위함이다.
   */
  if (signal(SIGINT, CHAN_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGINT) - %m\n");
    ret = -1;
    goto out;
  }
  if (signal(SIGTERM, CHAN_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGTERM) - %m\n");
    ret = -1;
    goto out;
  }

  /*
   * 현재 접속 중인 채널을 확인해서 화면에 출력한다.
   */
  if (g_op == kOperationType_check) {
    ret = CHAN_PrintCurrentChannel();
    if (ret < 0) {
      goto out;
    }
  }
  /*
   * 채널 접속을 수행한다.
   */
  else if (g_op == kOperationType_config) {
    ret = CHAN_AccessChannel(g_if_idx, g_chan_num[0], g_chan_num[1]);
    if (ret < 0) {
      goto out;
    }
  }

  if (g_loop == true) {
    while(1) {
      sleep(1);
    }
  }

  ret = 0;

out:
  /*
   * 접속계층라이브러리를 해제한다.
   */
  WAL_Close();

  return ret;
}
