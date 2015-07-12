#ifndef __UDP_H
#define __UDP_H

#include "datatype.h"
#include "IP.h"

/**
 * @brief UDP�ײ� 
 */
typedef struct UDPHeadStruct { 
	UINT16 srcPort;  /* Դ�˿ں� */
	UINT16 dstPort;  /* Ŀ��˿ں� */
	UINT16 length;   /* �ײ�+���ݵĳ��� */
	UINT16 checkSum; /* У��� */
} UDPHeadStruct;

/**
 * @brief UDPα�ײ� 
 */
typedef struct UDPPseudoHeadStruct {
	UINT8  srcIP[4]; /* ԴIP��ַ */
	UINT8  dstIP[4]; /* Ŀ��IP��ַ */
	UINT8  zero;     /* ȫ���ֶ� */
	UINT8  protocol; /* Э�飬��ӦIP_PROTOCOL_UDP */
	UINT16 length;   /* UDP�ײ�+���ݳ��� */
} UDPPseudoHeadStruct;


///* ��ǰ��������UDP����״̬������ */
//typedef struct UDPStruct {
//	UINT8  number;    /* �û�������UDP�����������û�н�����Ϊ0 */
//	UINT16 portTal[]; /* �˿ڱ� */
//} UDPStruct;

void UDP_process(IPHeadStruct *ip, UINT32 *offset);
mIPErr UDP_send(UINT16 srcPort, UINT8 *dstIP, UINT16 dstPort, UINT8 *data, UINT32 dataLen);

#endif
