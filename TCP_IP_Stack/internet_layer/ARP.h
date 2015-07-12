#ifndef __ARP_H
#define __ARP_H

#include "datatype.h"

typedef struct ARPHeadStruct {
	UINT16 hardwareType;    /* Ӳ������ */
	UINT16 protocolType;    /* �߲��Э������ */
	UINT8  hardwareSize;    /* Ӳ����ַ���ȣ�һ��Ϊ6�ֽ� */
	UINT8  protocolSize;    /* �߲�Э�鳤�� */
	UINT16 opCode;          /* �������ͣ�request:0x0001  reply:0x0002 */
	UINT8  senderMACAdd[6]; /* ���ͷ�MAC��ַ */
	UINT8  senderIPAdd[4];  /* ���ͷ�IP��ַ */
	UINT8  targetMACAdd[6]; /* Ŀ��MAC��ַ */
	UINT8  targetIPAdd[4];  /* Ŀ��IP��ַ */
} ARPHeadStruct;

void ARP_process(UINT32 *offset);
mIPErr ARP_request(UINT8 *dstIP, UINT8 *macBuf);
UINT8 ARP_checkCache(UINT8 *dstIP, UINT8 *macBuf);
void ARP_addCache(UINT8 *addIP, UINT8 *macBuf);
mIPErr ARP_getMacByIp(UINT8 *dstIP, UINT8 *macBuf);

#endif
