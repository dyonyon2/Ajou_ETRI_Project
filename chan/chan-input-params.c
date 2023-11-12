/**
 * @file
 * @brief chan 유틸리티 실행 시 함께 입력되는 입력 파라미터들을 파싱 및 저장하는 기능을 구현한 파일
 * @date 2019-08-15
 * @author gyun
 */


// 시스템 헤더 파일
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 라이브러리 헤더 파일
#include "wlanaccess/wlanaccess.h"

// 유틸리티 헤더 파일
#include "chan.h"


/**
 * @brief 옵션식별값에 따라 각 옵션을 처리한다.
 * @param[in] option_idx 옵션식별값 (struct option 의 4번째 멤버변수)
 */
static void CHAN_ProcessParsedOption(int option_idx)
{
  switch (option_idx) {
    case 0: {
      g_if_idx = (uint8_t)strtoul(optarg, 0, 10); // 통신인터페이스 식별번호
      break;
    }
    case 1: {
      g_chan_num[0] = (WalChannelNumber)strtol(optarg, 0, 10); // TimeSlot0 접속 채널
      break;
    }
    case 2: {
      g_chan_num[1] = (WalChannelNumber)strtol(optarg, 0, 10); // TimeSlot1 접속 채널
      break;
    }
    case 3: {
      g_loop = true; // 프로그램 종료 방지
      break;
    }
    default: {
      break;
    }
  }
}


/**
 * @brief 유틸리티 실행 시 함께 입력된 파라미터들을 파싱하여 관리정보에 저장한다.
 *
 * @param[in] argc 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 개수 (유틸리티 실행파일명 포함)
 * @param[in] argv 유틸리티 실행 시 입력되는 명령줄 내 파라미터들의 문자열 집합 (유틸리티 실행파일명 포함)
 * @retval 0: 성공
 * @retval -1: 실패
 */
int CHAN_ParsingInputParameters(int argc, char *argv[])
{
  int c, option_idx = 0;
  struct option options[] = {
    {"if", required_argument, 0, 0/*=getopt_long() 호출 시 option_idx 에 반환되는 값*/},
    {"ts0", required_argument, 0, 1},
    {"ts1", required_argument, 0, 2},
    {"loop", no_argument, 0, 3},
    {0, 0, 0, 0} // 옵션 배열은 {0,0,0,0} 센티넬에 의해 만료된다.
  };

  /*
   * 기본 파라미터 파싱 및 저장
   */
  if (!memcmp(argv[1], "check", 5)) {
    g_op = kOperationType_check;
  } else if (!memcmp(argv[1], "config", 6)) {
    g_op = kOperationType_config;
  } else {
    printf("Invalid operation - %s\n", argv[1]);
    return -1;
  }

  /*
   * 파라미터 기본 값 설정
   */
  g_if_idx = DEFAULT_IF_IDX;
  g_chan_num[0] = DEFAULT_CHAN_NUM;
  g_chan_num[1] = DEFAULT_CHAN_NUM;

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
     * 파싱된 옵션 처리 -> 저장
     */
    CHAN_ProcessParsedOption(option_idx);
  }

  return 0;
}
