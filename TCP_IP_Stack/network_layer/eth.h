#ifndef __ETH_H
#define __ETH_H

#include "datatype.h"

typedef struct ETHHeadStruct {
	UINT8 dstAdd[6]; /* Ŀ���ַ */
	UINT8 srcAdd[6]; /* Դ��ַ */
	UINT16 type;     /* �ϲ�Э������ */
} ETHHeadStruct;

mIPErr ETH_analyzeHead(ETHHeadStruct *ptr, UINT32 *offset);
void  ETH_makeHead(ETHHeadStruct *ptr, UINT32 *offset);
void  ETH_makeHeadDefault(UINT32 *offset);

#endif
