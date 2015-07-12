#ifndef __TCP_H
#define __TCP_H

#include "datatype.h"
#include "IP.h"
#include "mIPConfig.h"

/**
 * @brief TCP�ײ�
 */
typedef struct TCPHeadStruct {
	UINT16 srcPort;  /* Դ�˿� */
	UINT16 dstPort;  /* Ŀ�Ķ˿� */
	UINT32 squNum;   /* ���,ÿ���ֽڶ��ᱻ��� */
	UINT32 ackNum;   /* ȷ�Ϻ�,��ʾ�������յ���һ�����Ķε���� */
	UINT8  headLen;  /* 4bit�� TCP�ײ����ȣ���λΪ4byte */

	/* headLen��flag֮����6bit�ı������ڴ�ռλ */

	UINT8  flag;     /* 6bit���ֱ����URG, ACK, PSH, RST, SYN, FIN */
	UINT32 window;   /* ���ڣ�ָ���˶Է������͵�������,Ҳ�����Լ��Ļ����ܽ��ն��������� */
	UINT16 checkSum; /* У��ͣ���UDPһ������������ҲҪУ�飬��Ҫ����α�ײ� */
	UINT16 urgPtr;   /* ����ָ�룬��ʾ�����Ķ��н������ݵ��ֽ���,��������֮��Ϊ��ͨ���ݣ���ʹ����Ϊ0Ҳ�ܷ������� */
	
	UINT8  option[40];  /* ��ѡѡ��bufferָ�룬���Ȳ��̶������40byte */
	UINT32 MSS;      /* ����ĳ��ȣ���option�з��������ģ�Ĭ��536 */
} TCPHeadStruct;

/**
 * @brief TCPα�ײ�(ʵ���Ϻ�UDPα�ײ���һ���ģ�����Э��ΪIP���⣬Ϊ�˴���ṹ�򵥵㻹��������һ��) 
 */
typedef struct TCPPseudoHeadStruct {
	UINT8  srcIP[4]; /* ԴIP��ַ */
	UINT8  dstIP[4]; /* Ŀ��IP��ַ */
	UINT8  zero;     /* ȫ���ֶ� */
	UINT8  protocol; /* Э�飬��ӦIP_PROTOCOL_TCP */
	UINT16 length;   /* TCP�ײ�+���ݳ��� */
} TCPPseudoHeadStruct;

/**
 * @brief TCP������Ϣ���û�ָ���Ļص�����ʹ��
 */
typedef struct TCPInfoStruct {
	UINT8  srcIP[4];  /* ԴIP */
	UINT16 srcPort;   /* Դ�˿� */
	UINT16 dstPort;   /* Ŀ��˿� */
	UINT8  *data;     /* ����ָ�� */
	UINT32 dataLen;   /* ���ݳ��� */
	UINT32 squNumRcv; /* �ͻ��˷���������� */
	UINT32 ackNumRcv; /* �ͻ��˷�������ȷ�Ϻ� */
	UINT32 squNumSnd; /* �ϴα������ͳ�ȥ����� */
	UINT32 ackNumSnd; /* �ϴα������ͳ�ȥ��ȷ�Ϻ� */
	UINT32 serverWnd; /* ����ʣ�ര�ڴ�С */
	UINT32 clientWnd; /* �ͻ���ʣ�ര�ڴ�С */
	UINT32 clientMSS; /* �ͻ���MSS��С */
} TCPInfoStruct;

/**
 * @brief TCP����ṹ�壬��¼���������ӵ���Ϣ����ϵͳά�������ǿռ���Ҫ�ջ�����
 */
typedef struct TCPTaskStrcut {
	UINT8         state[TCP_COON_MAXNUM];  /* ���ӵ�״̬ */
	TCPInfoStruct info[TCP_COON_MAXNUM];   /* ��Ϣָ�� */
} TCPTaskStruct;


void   TCP_process(IPHeadStruct *ip, UINT32 *offset);
mIPErr TCP_send(UINT8 *dstIP, UINT16 srcPort, UINT16 dstPort, UINT32 seqNum, UINT32 ackNum, UINT8 headLen, UINT8 flag, UINT16 window, UINT16 urgPtr, UINT8 *option, UINT8 *data, UINT32 dataLen);
void   TCP_createTask(TCPTaskStruct *ptr, void (*cbPtr)(TCPInfoStruct *));
mIPErr TCP_replyMultiStart(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_replyMulti(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_replyAndWaitAck(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_reply(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_coonClose(TCPInfoStruct *info);
mIPErr TCP_waitAck(TCPInfoStruct *info, UINT8 mode);
mIPErr TCP_coonRelease(TCPInfoStruct *info);
mIPErr TCP_coonCloseAck(TCPInfoStruct *info);
UINT32 TCP_getInfoPos(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort);
void TCP_coonReset(void);

void coonEstabCallBack(TCPInfoStruct *info);
void coonSynCallBack(void);

#endif
