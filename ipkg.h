#include "idata.h"
#include "iqueue.h"

//=====================================================================
// SEGMENT
//=====================================================================
struct IKCPSEG
{
	struct IQUEUEHEAD node; // 用于串成队列。实际发送和接收的时候，自动去掉
	IUINT32 conv;           // 表示会话的编号，通信双方需要保证 conv 相同
	IUINT32 cmd;            // 用来区分分片类型

/*
const IUINT32 IKCP_CMD_PUSH = 81;		// cmd: push data           传输的数据包
const IUINT32 IKCP_CMD_ACK  = 82;		// cmd: ack                 ACK包，类似于 TCP中的 ACK，通知对方收到了哪些包
const IUINT32 IKCP_CMD_WASK = 83;		// cmd: window probe (ask)  用来探测远端窗口大小
const IUINT32 IKCP_CMD_WINS = 84;		// cmd: window size (tell)  告诉对方自己窗口大小
*/

	IUINT32 frg;            // segment分片ID（在message中的索引，由大到小，0表示最后一个分片），???后面还有几个seg???
	IUINT32 wnd;            // 剩余接收窗口大小(接收窗口大小-接收队列大小)
	IUINT32 ts;             // message发送时刻的时间戳
	IUINT32 sn;             // message分片segment的序号 （???ACK 的确认序号）
	IUINT32 una;            // 待接收消息序号(接收滑动窗口左端)
	IUINT32 len;            // 数据长度
	IUINT32 resendts;       // 下次超时重传的时间戳
	IUINT32 rto;            // 该分片的超时重传等待时间
	IUINT32 fastack;        // 收到ack时计算的该分片被跳过的累计次数
	IUINT32 xmit;           // 发送分片的次数，每发送一次加一。
	char data[1];
};
