#include "ARP.h"
#include "myTCPIP.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ��� 
 */
extern myTCPIPStruct mIP;

/**
 * @brief ������ȫ�ֱ���������ʱ�洢���յ�MAC��ַ  
 */
extern ETHHeadStruct ETHHead;


/**
 * @brief  ����ARP�ײ� 
 * @param  ptr: ARP�ײ��ṹ��ָ�룬���ս���������
 * @param  offset: bufferƫ����ָ��
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
static mIPErr ARP_analyzeHead(ARPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->hardwareType = MKUINT16BIG(buf);
	buf += 2;
	ptr->protocolType = MKUINT16BIG(buf);
	buf += 2;
	ptr->hardwareSize = *buf;
	buf += 1;
	ptr->protocolSize = *buf;
	buf += 1;
	ptr->opCode = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->senderMACAdd, buf, 6);
	buf += 6;
	bufCopy(ptr->senderIPAdd, buf, 4);
	buf += 4;
	bufCopy(ptr->targetMACAdd, buf, 6);
	buf += 6;
	bufCopy(ptr->targetIPAdd, buf, 4);
	buf += 4;
	*offset = buf - mIP.buf;

	return mIP_OK;
}

/**
 * @brief  ͨ��ARP�ײ��ṹ������ARP�ײ���buffer��
 * @param  ptr: ARP�ײ��ṹ��ָ��
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
static void ARP_makeHead(ARPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	UINT16TOPTRBIG(buf, ptr->hardwareType);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->protocolType);
	buf += 2;
	*buf++ = ptr->hardwareSize;
	*buf++ = ptr->protocolSize;
	UINT16TOPTRBIG(buf, ptr->opCode);
	buf += 2;
	bufCopy(buf, ptr->senderMACAdd, 6);
	buf += 6;
	bufCopy(buf, ptr->senderIPAdd, 4);
	buf += 4;
	bufCopy(buf, ptr->targetMACAdd, 6);
	buf += 6;
	bufCopy(buf, ptr->targetIPAdd, 4);	
	buf += 4;
	*offset = buf - mIP.buf;
}

/**
 * @brief  ARP��Ӧ�����ͱ���mac��ַ
 * @param  ptr: ARP�ײ��ṹ��
 * @retval none
 */
static void ARP_reply(ARPHeadStruct *ptr)
{
	ETHHeadStruct ethTmp;
	UINT32 offset = 0;
	
	/* ����ETHͷ */
	bufCopy(ethTmp.dstAdd, ptr->senderMACAdd, 6);
	bufCopy(ethTmp.srcAdd, mIP.mac, 6);
	ethTmp.type = PROTOCOL_TYPE_ARP;
	offset = 0;
	ETH_makeHead(&ethTmp, &offset);
	
	/* ����ARPͷ */
	ptr->opCode = ARP_OPCODE_REPLY; /* ��Ӧ���� */
	bufCopy(ptr->targetMACAdd, ptr->senderMACAdd, 6);
	bufCopy(ptr->targetIPAdd, ptr->senderIPAdd, 4);
	bufCopy(ptr->senderMACAdd, mIP.mac, 6);
	bufCopy(ptr->senderIPAdd, mIP.ip, 4);
	ARP_makeHead(ptr, &offset);
	/* ����ARP��Ӧ */
	myTCPIP_sendPacket(mIP.buf, offset);
}     

/**
 * @brief  ����һ��ARP��ѯ��Ȼ��ȴ���Ӧ��ʱ
 * @param  dstIP: Ŀ��IP��ַ
 * @param  macBuf: ��������Ŀ��IP��Ӧ��MAC��ַ
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
mIPErr ARP_request(UINT8 *dstIP, UINT8 *macBuf)
{
	ETHHeadStruct ethTmp;
	ARPHeadStruct arp;
	UINT32 offset = 0, len = 0, time = 0;
	UINT8  dstEthMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	UINT8  dstArpMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	/* ����ETHͷ */
	bufCopy(ethTmp.dstAdd, dstEthMac, 6);
	bufCopy(ethTmp.srcAdd, mIP.mac, 6);
	ethTmp.type = PROTOCOL_TYPE_ARP;
	offset = 0;
	ETH_makeHead(&ethTmp, &offset);
	
	/* ����ARPͷ */
	arp.hardwareType = HARDWARE_TYPE_ETH;
	arp.protocolType = PROTOCOL_TYPE_IP;
	arp.hardwareSize = 6;
	arp.protocolSize = 4;
	arp.opCode       = ARP_OPCODE_REQ; /* ���� */
	bufCopy(arp.senderMACAdd, mIP.mac, 6);
	bufCopy(arp.senderIPAdd, mIP.ip, 4);
	bufCopy(arp.targetMACAdd, dstArpMac, 6);
	bufCopy(arp.targetIPAdd, dstIP, 4);
	ARP_makeHead(&arp, &offset);	
    /* ����ARP��ѯ */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	/* �ȴ������Ļ�Ӧ */
	time = myTCPIP_getTime();
	while(1) {
		len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
		if(len == 0) {
			if(myTCPIP_getTime() - time > ARP_TIMEWAIT || 
			mIP.arpCache.arpUpdataTime > myTCPIP_getTime()) { /* ����ȴ���ʱ��ʱ�����������Ϊû�д����� */
				return mIP_NOACK;
			}
			continue;
		}
		
		offset = 0; /* ����buf��ƫ����Ϊ0 */
		ETH_analyzeHead(&ETHHead, &offset);
		if(ETHHead.type == PROTOCOL_TYPE_ARP) { /* ARPЭ�� */
			ARP_analyzeHead(&arp, &offset);
			if(arp.opCode == ARP_OPCODE_REPLY) { /* ����ǻ�Ӧ���� */
				if(bufMatch(arp.targetIPAdd, mIP.ip, 4)) { /* ��������IP�뱾����ƥ�� */
					bufCopy(macBuf, arp.senderMACAdd, 6);
					break;
				}
			}
		}
	}

	return mIP_OK;
}

/**
 * @brief  ���ARP���ٻ������Ƿ���dstIP��Ӧ��MAC��ַ��������Ƿ���Ҫ���»��� 
 * @param  dstIP: Ŀ��IP
 * @param  macBuf: ����Ŀ��IP��Ӧ��MAC��ַ
 * @retval 1: ��ѯ����ȡ�ɹ� 0��Ŀ��IP���ڻ�����
 */
UINT8 ARP_checkCache(UINT8 *dstIP, UINT8 *macBuf)
{
	UINT16 i;
	
	/* ����Ƿ񻺴��Ƿ���� */
	if(myTCPIP_getTime() - mIP.arpCache.arpUpdataTime > (ARP_CACHE_UPDATETIME * 1000) || 
	mIP.arpCache.arpUpdataTime > myTCPIP_getTime()) { /* ������˻������ʱ����߶�ʱ���������ջ��� */
		mIP.arpCache.num = 0;
		mIP.arpCache.arpUpdataTime = myTCPIP_getTime(); /* ���»���ʱ�� */
		return 0;
	}
	
	/* ��黺�����Ƿ��ж�ӦIP */
	for(i = 0; i < mIP.arpCache.num; i++) {
		if(bufMatch(dstIP, mIP.arpCache.ip[i], 4)) {
			bufCopy(macBuf, mIP.arpCache.mac[i], 6);
			return 1;
		}
	}
	
	return 0;
}

/**
 * @brief  ���һ��IP��MAC��ARP���ٻ�����,���������������ջ��沢��ӵ���ʼλ�� 
 * @param  addIP: ��ӵ�IP
 * @param  macBuf: ��ӦIP��MAC��ַ
 * @retval none
 */
void ARP_addCache(UINT8 *addIP, UINT8 *macBuf)
{
	if(mIP.arpCache.num >= ARP_CACHE_MAXNUM) { /* ��������˻������������Ŀ�������û�����ĿΪ0(��ջ���) */
		mIP.arpCache.num = 0;
		mIP.arpCache.arpUpdataTime = myTCPIP_getTime(); /* ���»���ʱ�� */
	}
	/* �Ѹղ�ѯ��IP��MAC��Ϣ��¼�������� */
	bufCopy(mIP.arpCache.ip[mIP.arpCache.num], addIP, 4);
	bufCopy(mIP.arpCache.mac[mIP.arpCache.num], macBuf, 6);
	mIP.arpCache.num++;
}

/**
 * @brief  ��ȡdstIP��Ӧ��MAC��ַ��������ARP���ٻ����в��ң�����У��ɵ�ַ���Ƶ�macBuf����û�У�����һ��ARP��ѯ����д�뻺�� 
 * @param  dstIP: Ŀ��IP
 * @param  macBuf: ����Ŀ��IP��Ӧ��MAC��ַ
 * @retval 0: �ɹ� 1�������ڴ����� 
 */
mIPErr ARP_getMacByIp(UINT8 *dstIP, UINT8 *macBuf)
{
	if(mIP.arpCache.num == 0 || !ARP_checkCache(dstIP, macBuf)) { /* ��������в��������ݻ��Ҳ�����ӦIP, ����ARP��ѯ */
		if(ARP_request(dstIP, macBuf) == mIP_OK) {          
			ARP_addCache(dstIP, macBuf);
		}
		else { /* ����������ϲ�����Ŀ��IP��Ӧ����������ʧ�� */
			return mIP_ERROR;
		}
	}
	
	return mIP_OK;
}

/**
 * @brief  ARP������Ҫ�����Զ���Ӧ�ͻ��˵�ARP����
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void ARP_process(UINT32 *offset)
{
	ARPHeadStruct arp;
	
	ARP_analyzeHead(&arp, offset);
    if(arp.opCode == ARP_OPCODE_REQ) { /* ������������ */
		if(bufMatch(arp.targetIPAdd, mIP.ip, 4)) { /* ��������IP�뱾����ƥ�� */
			ARP_reply(&arp);
		}
	}
}

