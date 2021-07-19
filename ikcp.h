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


//=====================================================================
// 32BIT INTEGER DEFINITION 
//=====================================================================
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
	defined(_M_AMD64)
	typedef unsigned int ISTDUINT32;
	typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
	defined(__i386) || defined(_M_X86)
	typedef unsigned long ISTDUINT32;
	typedef long ISTDINT32;
#elif defined(__MACOS__)
	typedef UInt32 ISTDUINT32;
	typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
	#include <sys/types.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
	#include <sys/inttypes.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
	typedef unsigned __int32 ISTDUINT32;
	typedef __int32 ISTDINT32;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#else 
	typedef unsigned long ISTDUINT32; 
	typedef long ISTDINT32;
#endif
#endif


//=====================================================================
// Integer Definition
//=====================================================================
#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef char IINT8;
#endif

#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char IUINT8;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef short IINT16;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 IUINT32;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 IUINT64;
#else
typedef unsigned long long IUINT64;
#endif
#endif

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


//=====================================================================
// QUEUE DEFINITION                                                  
//=====================================================================
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

// HEAD <--------------------------------------
//  |                                         |
//  v                                         |
//  [HEAD_NODE] <-> [NODE1] <->  ... <-> [NODE_N] -|
struct IQUEUEHEAD {
	struct IQUEUEHEAD *next, *prev;
};

typedef struct IQUEUEHEAD iqueue_head;


//---------------------------------------------------------------------
// queue init                                                         
//---------------------------------------------------------------------
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }
#define IQUEUE_HEAD(name) \
	struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

// 初始化队列，让 next 和 prev 都指向自己
#define IQUEUE_INIT(ptr) ( \
	(ptr)->next = (ptr), (ptr)->prev = (ptr))

// 计算 TYPE 结构体中 MEMBER 的偏移量
#define IOFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

// 指针前移至上一层的PKG (上一层为 TYPE 类型，本数据包为上层的 MEMBER)
#define ICONTAINEROF(ptr, type, member) ( \
		(type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

// 指针前移至上一层的PKG (上一层为 TYPE 类型，本数据包为上层的 MEMBER)
#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


//---------------------------------------------------------------------
// queue operation  
// 队列操作                   
//---------------------------------------------------------------------
// 头插
#define IQUEUE_ADD(node, head) ( \
	(node)->prev = (head), (node)->next = (head)->next, \
	(head)->next->prev = (node), (head)->next = (node))

// 尾插
#define IQUEUE_ADD_TAIL(node, head) ( \
	(node)->prev = (head)->prev, (node)->next = (head), \
	(head)->prev->next = (node), (head)->prev = (node))

// 短接两个节点，即“删除”中间节点
#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

// 删除 entry 节点，短接前后节点
#define IQUEUE_DEL(entry) (\
	(entry)->next->prev = (entry)->prev, \
	(entry)->prev->next = (entry)->next, \
	(entry)->next = 0, (entry)->prev = 0)

// 删除 entry 然后让 entry 自成一个新的双端队列
#define IQUEUE_DEL_INIT(entry) do { \
	IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0)

// next 指向自己，队列即为空
#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

#define iqueue_init		IQUEUE_INIT
#define iqueue_entry	IQUEUE_ENTRY
#define iqueue_add		IQUEUE_ADD
#define iqueue_add_tail	IQUEUE_ADD_TAIL
#define iqueue_del		IQUEUE_DEL
#define iqueue_del_init	IQUEUE_DEL_INIT
#define iqueue_is_empty IQUEUE_IS_EMPTY

// 遍历队列
#define IQUEUE_FOREACH(iterator, head, TYPE, MEMBER) \
	for ((iterator) = iqueue_entry((head)->next, TYPE, MEMBER); \
		&((iterator)->MEMBER) != (head); \
		(iterator) = iqueue_entry((iterator)->MEMBER.next, TYPE, MEMBER))

// 遍历队列
#define iqueue_foreach(iterator, head, TYPE, MEMBER) \
	IQUEUE_FOREACH(iterator, head, TYPE, MEMBER)

// 遍历队列
#define iqueue_foreach_entry(pos, head) \
	for( (pos) = (head)->next; (pos) != (head) ; (pos) = (pos)->next )
	

#define __iqueue_splice(list, head) do {	\
		iqueue_head *first = (list)->next, *last = (list)->prev; \
		iqueue_head *at = (head)->next; \
		(first)->prev = (head), (head)->next = (first);		\
		(last)->next = (at), (at)->prev = (last); }	while (0)

#define iqueue_splice(list, head) do { \
	if (!iqueue_is_empty(list)) __iqueue_splice(list, head); } while (0)

#define iqueue_splice_init(list, head) do {	\
	iqueue_splice(list, head);	iqueue_init(list); } while (0)


#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

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


// RTT: 发送一个数据包到收到对应的ACK，所花费的时间
// RTO: 发送数据包，启动重传定时器，重传定时器到期所花费的时间

//---------------------------------------------------------------------
// IKCPCB
//---------------------------------------------------------------------

// 接收窗口: recv_que 
// 对于每个收到的seg，都会返回ACK
struct IKCPCB
{
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


