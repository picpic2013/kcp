#include "ipcc.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
// --------------------------------------------------------------------
// PCC Util functions
// --------------------------------------------------------------------
static inline double isigmoid(double x, double alpha) {
  return 1 / (1 + exp(alpha * x));
}

// alpha should larger than MAX{2.2 * (n - 1), 100)}
static inline double utilityFunction(unsigned int sendingRate, double lossRate, double alpha = 22) {
  double Ti = sendingRate * (1 - lossRate);
  return Ti * isigmoid(lossRate - 0.05, alpha) - (double)sendingRate * lossRate; 
}

// 将PCC的cwnd转换为包单位
static inline unsigned int calculateCwnd(struct IQUEUEHEAD *head, unsigned int cwndInByte) {

  if (IQUEUE_IS_EMPTY(head)) {
    return 0;
  }

  iqueue_head *p;
  unsigned int result = 0;

  for (p = head->next; p != head; p = p->next) {
    struct IKCPSEG *seg = IQUEUE_ENTRY(p, struct IKCPSEG, node);

    if (seg->len > cwndInByte) {
      break;
    }

    ++result;
    cwndInByte -= seg->len;
  }

  return result;
}

static struct IPCCNODE* ipcc_seg_new() {
  return (struct IPCCNODE*)malloc(sizeof(struct IPCCNODE));
}

void initPCC(ipcccb *pcc, unsigned int startCwnd, unsigned int startEpsilon) {
  iqueue_init(&pcc->exp_buf);
  pcc->state = PCC_START_STAGE;
  pcc->nRA = 0;
  pcc->nexp = 0;
  pcc->nexpRx = 10086;
  pcc->cwnd = startCwnd;
  pcc->cwndLast = 0;

  pcc->bestScore = -1e100;
  pcc->txExpCnt = 0;
  pcc->rxExpCnt = 0;

  pcc->expScore[0] = pcc->expScore[1] = pcc->expScore[2] = pcc->expScore[3] = 0;
  pcc->expCmd[0] = pcc->expCmd[1] = 1;
  pcc->expCmd[2] = pcc->expCmd[3] = -1;
  pcc->expSn[0] = pcc->expSn[1] = pcc->expSn[2] = pcc->expSn[3] = 0;

  pcc->dir = 1;
  pcc->startEpsilon = pcc->epsilon = startEpsilon;
}

double getScore(ipcccb *pcc, unsigned int expSn) {
  if (iqueue_is_empty(&pcc->exp_buf)) {
    return utilityFunction(0, 0);
  }
   
  struct IPCCNODE *node;
  unsigned int throughput = 0, lossCnt = 0;

  for (iqueue_head *p = pcc->exp_buf.next; p != &pcc->exp_buf; p = p->next) {
    node = iqueue_entry(p, struct IPCCNODE, node);

    if (node->nexp == expSn) {
      throughput += 1;
      
      if (node->ack == 0) {
        lossCnt += 1;
      }

      iqueue_del(&node->node);
      free(node);
    }
  }

  return utilityFunction(throughput, (double)lossCnt / throughput);
}

// 初始化实验
void initExp(ipcccb *pcc) {
  // 初始化实验参数
  pcc->txExpCnt = 0;
  pcc->rxExpCnt = 0;
  pcc->expScore[0] = pcc->expScore[1] = pcc->expScore[2] = pcc->expScore[3] = 0;
  pcc->expSn[0] = pcc->expSn[1] = pcc->expSn[2] = pcc->expSn[3] = 0;

  pcc->dir = 1;
  pcc->epsilon = pcc->startEpsilon;
  
  // 规划四次实验
  // 随机打乱实验顺序
  srand((unsigned int)time(NULL));
  int rdm = rand() % 4;
  
  int swapTmp = pcc->expCmd[0];
  pcc->expCmd[0] = pcc->expCmd[rdm];
  pcc->expCmd[rdm] = swapTmp;
}

void recvAck(ipcccb *pcc, unsigned int sn) {
  // 维护ACK && expSn
  unsigned int nowExpSn = 0;
  struct IPCCNODE *node;
  for (iqueue_head *p = pcc->exp_buf.next; p != &pcc->exp_buf; p = p->next) {
    node = iqueue_entry(p, struct IPCCNODE, node);

    if (node->sn == sn) {
      node->ack = 1;
      nowExpSn = node->nexp;
      break;
    }
  }

  if (nowExpSn == pcc->nexpRx) {
    return;
  }
  pcc->nexpRx = nowExpSn;

  // 统计上一次实验的分数
  if (nowExpSn == 0) {
    return;
  }
  double nowScore = getScore(pcc, nowExpSn - 1); 

  // 状态机转+移维护最佳分数
  if (pcc->state == PCC_START_STAGE) {
    if (nowScore > pcc->bestScore) {
      pcc->cwndLast = pcc->cwnd;
      pcc->cwnd <<= 1; 
      pcc->bestScore = nowScore;
    } else {
      // 恢复上次的cwnd
      pcc->cwnd = pcc->cwndLast;

      // 初始化实验
      initExp(pcc);

      // 转换状态
      pcc->state = PCC_DECISION_MAKING_STAGE;
    }
  }
  
  else if (pcc->state == PCC_RATE_ADJUSTING_STAGE) {
    if (nowScore > pcc->bestScore) {
      pcc->cwndLast = pcc->cwnd;
      pcc->cwnd = (1 + pcc->dir * pcc->nRA * pcc->epsilon) * pcc->cwnd; 
      pcc->bestScore = nowScore;
    } else {
      // 恢复上次的cwnd
      pcc->cwnd = pcc->cwndLast;

      // 初始化实验
      initExp(pcc);

      // 转换状态
      pcc->state = PCC_DECISION_MAKING_STAGE;
    }
  }

  else if (pcc->state == PCC_DECISION_MAKING_STAGE) {
    if (pcc->rxExpCnt < 4) {
      // 根据 sn 找到实验序号
      // 网络报文可能乱序
      int expIdx = 0;
      for (int i = 0; i < 4; ++i) {
        if (pcc->expSn[i] == nowExpSn) {
          expIdx = i;
          break;
        }
      }

      pcc->expScore[expIdx] = nowScore;
      pcc->rxExpCnt += 1;
    } else {
      double posScore[2], negScore[2];
      unsigned int posCnt = 0, negCnt = 0;
      for (int i = 0; i < 4; ++i) {
        if (pcc->expCmd[i] == 1) {
          posScore[posCnt++] = pcc->expScore[i];
        }

        else if (pcc->expCmd[i] == -1) {
          negScore[negCnt++] = pcc->expScore[i];
        }
      }
      
      // 正常进行了4次实验
      if (posCnt == 2 && negCnt == 2) {
        // 正向分更高
        if (posScore[0] > negScore[0] && posScore[0] > negScore[1] &&
            posScore[1] > negScore[0] && posScore[1] > negScore[1]) {
          pcc->dir = 1;
          pcc->epsilon = pcc->startEpsilon;
  
          pcc->state = PCC_RATE_ADJUSTING_STAGE;
        }
        // 负向分更高
        else if (posScore[0] < negScore[0] && posScore[0] < negScore[1] &&
            posScore[1] < negScore[0] && posScore[1] < negScore[1]) {
          pcc->dir = -1;
          pcc->epsilon = pcc->startEpsilon;

          pcc->state = PCC_RATE_ADJUSTING_STAGE;
        }

        // 无法决定
        else {
          initExp(pcc);
          pcc->epsilon = pcc->epsilon + PCC_MIN_EPSILON;
          if (pcc->epsilon > PCC_MAX_EPSILON) {
            pcc->epsilon = PCC_MAX_EPSILON;
          }
        }
      }

      else {
        exit(-2);
      }

    }
  }


}

void sendPkg(ipcccb *pcc, unsigned int sn) {
  struct IPCCNODE *newNode = ipcc_seg_new();

  newNode->sn = sn;
  newNode->nexp = pcc->nexp;
  newNode->ack = 0;

  iqueue_add_tail(&newNode->node, &pcc->exp_buf);

  if (pcc->state == PCC_DECISION_MAKING_STAGE && pcc->txExpCnt < 4) {
    if (pcc->txExpCnt < 4) {
      pcc->expSn[pcc->txExpCnt] = pcc->nexp;
      pcc->cwnd = (1 + pcc->expCmd[pcc->txExpCnt] * pcc->epsilon) * pcc->cwnd;
      ++pcc->txExpCnt;
    } else {
      pcc->cwnd = pcc->cwndLast;
    }
  }
}

void startNewExp(ipcccb *pcc) {
  pcc->nexp += 1;
}

unsigned int getCwnd(ipcccb *pcc) {
  return pcc->cwnd; 
}
