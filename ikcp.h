//=====================================================================
//
// KCP - A Better ARQ Protocol Implementation
// skywind3000 (at) gmail.com, 2010-2011
//  
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
//=====================================================================
#ifndef __IKCP_H__
#define __IKCP_H__

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "iqueue.h"
#include "ipcc.h"
#include "idata.h"
#include "iqueue.h"

#ifndef INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#if (!defined(__cplusplus)) && (!defined(inline))
#define inline INLINE
#endif


//---------------------------------------------------------------------
// BYTE ORDER & ALIGNMENT
//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN
    #ifdef _BIG_ENDIAN_
        #if _BIG_ENDIAN_
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MIPSEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #define IWORDS_BIG_ENDIAN  0
    #endif
#endif

#ifndef IWORDS_MUST_ALIGN
	#if defined(__i386__) || defined(__i386) || defined(_i386_)
		#define IWORDS_MUST_ALIGN 0
	#elif defined(_M_IX86) || defined(_X86_) || defined(__x86_64__)
		#define IWORDS_MUST_ALIGN 0
	#elif defined(__amd64) || defined(__amd64__)
		#define IWORDS_MUST_ALIGN 0
	#else
		#define IWORDS_MUST_ALIGN 1
	#endif
#endif

// RTT: 发送一个数据包到收到对应的ACK，所花费的时间
// RTO: 发送数据包，启动重传定时器，重传定时器到期所花费的时间

//---------------------------------------------------------------------
// IKCPCB
//---------------------------------------------------------------------

// 接收窗口: recv_que 
// 对于每个收到的seg，都会返回ACK
struct IKCPCB
{
	// PCC_Control_Block
  struct PCCCB pcccb;

  // conv: 表示会话的编号，通信双方需要保证 conv 相同
	// mtu: 最大传输单元，包或帧的最大长度
	// mss: 通信时每一个报文段所能承载的最大数据长度
	// state: 如果一个包的重发次数大于 dead_link ，state 将被设置为 (IUINT32)-1
	IUINT32 conv, mtu, mss, state;

	// snd_una: 已发送但尚未确认的数据的第一个字节的序列号
	// snd_nxt: 下一个要送入 buf 的 seg sn
	// rcv_nxt: recv_queue下一个想要的有序seg sn 
	IUINT32 snd_una, snd_nxt, rcv_nxt;

	// ts_recent: 
	// ts_lastack: 
	// ssthresh: 慢启动的拥塞窗口阈值
	IUINT32 ts_recent, ts_lastack, ssthresh;
	
	// rx_rttval: 平均 rtt
	// rx_srtt: 瞬时 rtt 发送一个数据包到收到对应的ACK，所花费的时间
	// rx_rto:  瞬时 rto 发送数据包，启动重传定时器，重传定时器到期所花费的时间
	// rx_minrto: 最小 rto
	IINT32 rx_rttval, rx_srtt, rx_rto, rx_minrto;
	
	// snd_wnd: 
	// rcv_wnd:
	// rmt_wnd: 远端接收窗口大小 (为0则停止发送，一段时间后询问)
	// cwnd: 
	// probe: flush中需要执行的指令
/*
	const IUINT32 IKCP_ASK_SEND = 1;		// need to send IKCP_CMD_WASK
	const IUINT32 IKCP_ASK_TELL = 2;		// need to send IKCP_CMD_WINS
	const IUINT32 IKCP_WND_SND = 32;
	const IUINT32 IKCP_WND_RCV = 128;       // must >= max fragment size
*/
	IUINT32 snd_wnd, rcv_wnd, rmt_wnd, cwnd, probe;

	// current: 当前时间戳（毫秒）ikcp_update函数中更新
	// interval: 
	// ts_flush: 下一次需要调用 flush 的时间戳 （该变量在ikcp_update函数中更新&设置）
	// xmit:  
	IUINT32 current, interval, ts_flush, xmit;
	
	// nrcv_buf: rcv_buf 中的包个数
	// nsnd_buf: snd_buf 中的包个数
	IUINT32 nrcv_buf, nsnd_buf;
	
	// nrcv_que: rcv_que 中的包个数
	// nsnd_que: snd_que 中的包个数
	IUINT32 nrcv_que, nsnd_que;
	
	// nodelay: 
	// updated: 此周期是否已经调用过 ikcp_update
	IUINT32 nodelay, updated;
	
	// ts_probe: 下一次探测远端接收窗口的时间戳
	// probe_wait: 探测远端接收窗口的等待时间
	IUINT32 ts_probe, probe_wait;
	
	// dead_link: 重发次数最大阈值。大于阈值，向state报告
	// incr: 拥塞窗口增量
	IUINT32 dead_link, incr;

	// snd_queue: 发送队列，此队列不维护sn之类的值
	struct IQUEUEHEAD snd_queue;
	
	// rcv_queue: 已经排好序的接收队列
	struct IQUEUEHEAD rcv_queue;

	// snd_buf: 已经发出但是没收到确认的队列
	struct IQUEUEHEAD snd_buf;

	// rcv_buf: 已接收但仍不完整的接收队列。排好序(sn小 --> sn大)
	struct IQUEUEHEAD rcv_buf;
	
	// acklist: ack list (长度为ackcount的两倍 偶数: sn, 奇数: ts)
	IUINT32 *acklist;

	// ackcount: ACK List 中实际有 ACK 的数量
	IUINT32 ackcount;

	// ackblock: ACK List 的实际长度
	IUINT32 ackblock;
	void *user;

	// flush 时发送 seg 的缓存
	char *buffer;

	// 是否 ack 被跳过多少次就重发（快重传）
	int fastresend;

	// 重传阈值
	int fastlimit;

	// nocwnd: ??? 忽略拥塞控制模式
	// stream: 流模式，即如果没达到 mss 就把新的包附加在上一个包后面，此模式 frg 无效
	int nocwnd, stream;
	int logmask;
	int (*output)(const char *buf, int len, struct IKCPCB *kcp, void *user);
	void (*writelog)(const char *log, struct IKCPCB *kcp, void *user);
};


typedef struct IKCPCB ikcpcb;

#define IKCP_LOG_OUTPUT			1
#define IKCP_LOG_INPUT			2
#define IKCP_LOG_SEND			4
#define IKCP_LOG_RECV			8
#define IKCP_LOG_IN_DATA		16
#define IKCP_LOG_IN_ACK			32
#define IKCP_LOG_IN_PROBE		64
#define IKCP_LOG_IN_WINS		128
#define IKCP_LOG_OUT_DATA		256
#define IKCP_LOG_OUT_ACK		512
#define IKCP_LOG_OUT_PROBE		1024
#define IKCP_LOG_OUT_WINS		2048

#ifdef __cplusplus
extern "C" {
#endif


//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------

// create a new kcp control object, 'conv' must equal in two endpoint
// from the same connection. 'user' will be passed to the output callback
// output callback can be setup like this: 'kcp->output = my_udp_output'
ikcpcb* ikcp_create(IUINT32 conv, void *user);

// release kcp control object
void ikcp_release(ikcpcb *kcp);

// set output callback, which will be invoked by kcp
void ikcp_setoutput(ikcpcb *kcp, int (*output)(const char *buf, int len, 
	ikcpcb *kcp, void *user));

// user/upper level recv: returns size, returns below zero for EAGAIN
int ikcp_recv(ikcpcb *kcp, char *buffer, int len);

// user/upper level send, returns below zero for error
int ikcp_send(ikcpcb *kcp, const char *buffer, int len);

// update state (call it repeatedly, every 10ms-100ms), or you can ask 
// ikcp_check when to call it again (without ikcp_input/_send calling).
// 'current' - current timestamp in millisec. 
void ikcp_update(ikcpcb *kcp, IUINT32 current);

// Determine when should you invoke ikcp_update:
// returns when you should invoke ikcp_update in millisec, if there 
// is no ikcp_input/_send calling. you can call ikcp_update in that
// time, instead of call update repeatly.
// Important to reduce unnacessary ikcp_update invoking. use it to 
// schedule ikcp_update (eg. implementing an epoll-like mechanism, 
// or optimize ikcp_update when handling massive kcp connections)
IUINT32 ikcp_check(const ikcpcb *kcp, IUINT32 current);

// when you received a low level packet (eg. UDP packet), call it
int ikcp_input(ikcpcb *kcp, const char *data, long size);

// flush pending data
void ikcp_flush(ikcpcb *kcp);

// check the size of next message in the recv queue
int ikcp_peeksize(const ikcpcb *kcp);

// change MTU size, default is 1400
int ikcp_setmtu(ikcpcb *kcp, int mtu);

// set maximum window size: sndwnd=32, rcvwnd=32 by default
int ikcp_wndsize(ikcpcb *kcp, int sndwnd, int rcvwnd);

// get how many packet is waiting to be sent
int ikcp_waitsnd(const ikcpcb *kcp);

// fastest: ikcp_nodelay(kcp, 1, 20, 2, 1)
// nodelay: 0:disable(default), 1:enable
// interval: internal update timer interval in millisec, default is 100ms 
// resend: 0:disable fast resend(default), 1:enable fast resend
// nc: 0:normal congestion control(default), 1:disable congestion control
int ikcp_nodelay(ikcpcb *kcp, int nodelay, int interval, int resend, int nc);


void ikcp_log(ikcpcb *kcp, int mask, const char *fmt, ...);

// setup allocator
void ikcp_allocator(void* (*new_malloc)(size_t), void (*new_free)(void*));

// read conv
IUINT32 ikcp_getconv(const void *ptr);


#ifdef __cplusplus
}
#endif

#endif


