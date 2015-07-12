#ifndef __IP_H
#define __IP_H

#include "datatype.h"

/**
 * @brief IPͷ�ṹ��
 */
typedef struct IPHeadStruct { /* IP�̶���20byte�ײ����������ɱ��ֶ� */
	UINT8  version;        /* 4bit IP�汾��һ��ΪIPV4��IPV6 */
	UINT8  headLength;     /* 4bit ͷ������: һ��20byte */
	UINT8  diffServices;   /* ���ַ���һ�㲻�� */
	UINT16 totalLength;    /* ���ݺ�ͷ���ܳ��� */
	UINT16 identification; /* ���ݰ���ʶ */
	UINT8  flag;           /* 3bit ��־ */
	UINT16 offset;         /* 13bit Ƭƫ��(8���ֽ�Ϊһ��ƫ�Ƶ�λ) */
	UINT8  timeToLive;     /* ����ʱ�� */
	UINT8  protocol;       /* Э�飬����ICMP��TCP�ȵ� */
	UINT16 checkSum;       /* ͷ��У��� */
	UINT8  srcAdd[4];      /* ԴIP��ַ */
	UINT8  dstAdd[4];      /* Ŀ��IP��ַ */	
} IPHeadStruct;


void IP_process(UINT32 *offset);
void IP_makeHead(IPHeadStruct *ptr, UINT32 *offset);
mIPErr IP_analyzeHead(IPHeadStruct *ptr, UINT32 *offset);
void IP_makeHeadDefault(UINT16 datLen, UINT16 identification, UINT8 flag, UINT16 fOffset, UINT8 protocol, UINT32 *offset);

#endif                    
