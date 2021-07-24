#ifndef __IPCC_H__
#define __IPCC_H__


#include "iqueue.h"
#include "idata.h"
#include "ipkg.h"

// PCC 维护的发送队列
struct IPCCNODE {

  // 队列指针
  struct IQUEUEHEAD node;

  // sn, 实验序号
  IUINT32 sn, nexp;

  // 是否已经收到ACK
  IUINT8 ack;
};

// PCC_Control_Block
#define PCC_START_STAGE            0
#define PCC_DECISION_MAKING_STAGE  1
#define PCC_RATE_ADJUSTING_STAGE   2

#define PCC_MIN_EPSILON 0.01
#define PCC_MAX_EPSILON 0.05

struct PCCCB {
  // 维护发送队列
  iqueue_head exp_buf;

  // 状态, 调整阶段发送的包数量, 实验ID, 接收实验ID, 拥塞窗口大小, 前一个cwnd
  unsigned int state, nRA, nexp, nexpRx, cwnd, cwndLast;

  // 最佳成绩
  double bestScore;

  // 发送实验次数, 接收试验次数
  int txExpCnt, rxExpCnt;

  // 四次实验分数, 实验增减, 实验ID
  int expCmd[4];
  unsigned int expSn[4];
  double expScore[4];
  
  // 速率变化方向
  int dir;

  // 速率变化步长
  double epsilon, startEpsilon;
};

typedef struct PCCCB ipcccb;


#ifdef __cplusplus
extern "C" {
#endif

void initPCC(ipcccb *pcc, unsigned int startCwnd, unsigned int startEpsilon);
double getScore(ipcccb *pcc, unsigned int expSn);
void recvAck(ipcccb *pcc, unsigned int sn);
void sendPkg(ipcccb *pcc, unsigned int sn);
void startNewExp(ipcccb *pcc);
static inline unsigned int calculateCwnd(struct IQUEUEHEAD *head, unsigned int cwndInByte) {
unsigned int getCwnd(ipcccb *pcc);


#ifdef __cplusplus
}
#endif

#endif
