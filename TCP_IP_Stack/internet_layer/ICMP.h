#ifndef __ICMP_H
#define __ICMP_H

#include "datatype.h"
#include "IP.h"

typedef struct ICMPHeadStruct {
	UINT8  type;           /* ICMP���� */
	UINT8  code;           /* ICMP���룬���������ʹ�� */
	UINT16 checkSum;       /* У��� */
	UINT16 identification; /* ��ʶ(���ѯ�ʱ���) */
	UINT16 sequence;       /* ���к�(���ѯ�ʱ���) */
	UINT8  *data;          /* ����ָ�� */	
} ICMPHeadStruct;

void ICMP_process(IPHeadStruct *ip, UINT32 *offset);

#endif
