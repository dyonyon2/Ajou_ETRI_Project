/**
 * @file
 * @brief chan 유틸리티 구현 메인 파일
 *
 * 본 유틸리티는 V2X 통신인터페이스의 MAC 주소 설정 및 확인 기능을 수행한다. \n
 * 본 유틸리티는 접속계층라이브러리 API를 사용한다.
 */


// 시스템 헤더 파일
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 라이브러리 헤더 파일
#include "wlanaccess/wlanaccess.h"


/// 디바이스가 지원하는 통신인터페이스 개수
int g_if_num;


/**
 * @brief 유틸리티 사용법을 화면에 출력한다.
 * @param[in] app_filename 어플리케이션 실행파일명
 */
static void ADDR_Usage(char app_filename[])
{
  printf("\n\n Description: Utility to check and configure the channel using v2x-sw libraries\n");

  printf("\n Usage: %s set|get <if_idx> [address]\n\n", app_filename);
  printf("          set: set MAC address of interface whose index is \"if_idx\". \"address\" must be specified\n");
  printf("          get: get MAC address of interface whose index is \"if_idx\"\n");
  printf("\n\n");
}


/**
 * @brief 종료신호 핸들러. WAL_Close()를 호출한다.
 * @param signo 사용되지 않음.
 */
static void ADDR_SigHandler(int signo)
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
   * 파라미터가 부족하면 사용법을 출력한다.
   */
  if (argc < 3) {
    ADDR_Usage(argv[0]);
    return 0;
  }

  printf("Running addr utility..\n");

  /*
   * 접속계층라이브러리를 초기화한다.
   */
  int ret = WAL_Open(kWalLogLevel_Err);
  if (ret < 0) {
    printf("Fail to open wlanaccess library - %d\n", ret);
    return -1;
  }
  g_if_num = ret;
  printf("Success to open wlanaccess library - %d interface supported\n", g_if_num);

  /*
   * Ctrl + C, Kill 신호에 대한 핸들러를 등록한다 - 종료 시 WAL_Close()를 호출하기 위함이다.
   */
  if (signal(SIGINT, ADDR_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGINT) - %m\n");
    ret = -1;
    goto out;
  }
  if (signal(SIGTERM, ADDR_SigHandler) == SIG_ERR) {
    printf("Fail to signal(SIGTERM) - %m\n");
    ret = -1;
    goto out;
  }

  /*
   * 입력된 통신인터페이스 식별번호의 유효성을 확인한다.
   */
  uint8_t if_idx = (uint8_t)strtoul(argv[2], NULL, 10);
  if (if_idx >= g_if_num) {
    printf("Invalid if_idx %u\n", if_idx);
    goto out;
  }

  /*
   * MAC 주소를 설정한다.
   */
  uint8_t addr[6];
  if (memcmp(argv[1], "set", 3) == 0) {
    WAL_ConvertMACAddressStrToOctets(argv[3], addr);
    ret = WAL_SetIfMACAddress(if_idx, addr);
    if (ret < 0) {
      printf("Fail to set MAC address for if[%u] as %02X:%02X:%02X:%02X:%02X:%02X - ret: %d\n",
        if_idx, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], ret);
    } else {
      printf("Success to set MAC address for if[%u] as %02X:%02X:%02X:%02X:%02X:%02X\n",
             if_idx, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
  }
  /*
   * MAC 주소를 확인한다.
   */
  else {
    ret = WAL_GetIfMACAddress(if_idx, addr);
    if (ret < 0) {
      printf("Fail to get MAC address for if[%u] - ret: %d\n", if_idx, ret);
    } else {
      printf("Success to get MAC address for if[%u] - %02X:%02X:%02X:%02X:%02X:%02X\n",
        if_idx, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    }
  }

  ret = 0;

out:
  /*
   * 접속계층 라이브러리를 해제한다.
   */
  WAL_Close();

  return ret;
}
