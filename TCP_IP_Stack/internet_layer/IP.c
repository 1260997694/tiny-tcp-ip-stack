#include "IP.h"
#include "myTCPIP.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ���
 */
extern myTCPIPStruct mIP;

/**
 * @brief  ����IPͷ
 * @param  ptr: IPͷ�ṹ��ָ�룬������Ž���������� 
 * @param  offset: ƫ����ָ�룬��ʾIPͷ��Ϣ��buffer�е�ƫ����
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
mIPErr IP_analyzeHead(IPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->version = (UINT8)(*buf >> 4);
	ptr->headLength = *buf++ & 0x0F;
    ptr->diffServices = *buf++;
	ptr->totalLength = MKUINT16BIG(buf);
	buf += 2;
	ptr->identification = MKUINT16BIG(buf);
	buf += 2;
	ptr->flag = *buf >> 5;
	ptr->offset = ((UINT16)(*buf & 0x1F) << 8) | (UINT16)(*(buf + 1));
	buf += 2;
	ptr->timeToLive = *buf++;
	ptr->protocol = *buf++;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->srcAdd, buf, 4);
	buf += 4;
	bufCopy(ptr->dstAdd, buf, 4);
	buf += 4;
    *offset += ptr->headLength * 4;
	
	return mIP_OK;
}

/**
 * @brief  ͨ��IPͷ�ṹ������IPͷ��buffer��
 * @param  ptr: IPͷ�ṹ��ָ��
 * @param  offset: buffer��ƫ����ָ�� 
 * @retval none
 */
void IP_makeHead(IPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT16 sum;
	
	*buf++ = (ptr->version << 4) | ptr->headLength;
	*buf++ = ptr->diffServices;
	UINT16TOPTRBIG(buf, ptr->totalLength);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->identification);
	buf += 2;
	UINT16TOPTRBIG( buf, (((UINT16)ptr->flag << 5) | (ptr->offset & 0x1FFF)) );
	buf += 2;
	*buf++ = ptr->timeToLive;
	*buf++ = ptr->protocol;
	UINT16TOPTRBIG(buf, 0x0000);  /* �ײ�У���������Ϊ0 */
	buf += 2;
	bufCopy(buf, ptr->srcAdd, 4);
	buf += 4;
	bufCopy(buf, ptr->dstAdd, 4);
	buf += 4;
	
	sum = calcuCheckSum(mIP.buf + *offset, ptr->headLength * 4); /* �����ײ�У��� */
	UINT16TOPTRBIG((mIP.buf + *offset + 10), sum);

	*offset = buf - mIP.buf;
}

/**
 * @brief  ����Ĭ��IPͷ��Ĭ�ϰ汾��ΪIPv4���ײ���СΪ20byte��ԴIPΪ����IP������ʱ��Ϊ128
 * @param  totalLength: ���ݵĴ�С 
 * @param  identification: ��ʶ����ά����mIP.identification�У��ڷ�Ƭ����ʱ��ͬһ��ʶ���Ա�����
 * @param  flag: ��־��ֻ��3λ��Ч��0x00��0x20��ʾ����û�з�Ƭ�� 0x01��ʾ���滹�з�Ƭ
 * @param  fOffset: ƫ��������ʾ��ͬ��Ƭ�������е�ƫ������ֻ��13λ��Ч����8byteΪ��λ������ÿ����Ƭ�ĳ���λ8�ֽڵ�������
 * @param  protocol: �߲�ʹ�õ�Э��
 * @param  offset: buffer��ƫ����ָ��
 * @retval none
 */
void IP_makeHeadDefault(UINT16 datLen, UINT16 identification, UINT8 flag, UINT16 fOffset, UINT8 protocol, UINT32 *offset)
{
	IPHeadStruct ip;
	ip.version = 4;
	ip.headLength = 5;
	ip.diffServices = 0x00;
	ip.totalLength = datLen + 20;
	ip.identification = identification;
	ip.flag = flag;
	ip.offset = fOffset;
	ip.timeToLive = 0x80;
	ip.protocol = protocol;
	ip.checkSum = 0x0000;
	bufCopy(ip.srcAdd, mIP.ip, 4);
	bufCopy(ip.dstAdd, mIP.ipTmp, 4);
	IP_makeHead(&ip, offset);         
}

/**
 * @brief  IP����
 * @param  offset: buffer��ƫ����ָ�� 
 * @retval none
 */
void IP_process(UINT32 *offset)
{
	IPHeadStruct ip;
	UINT16 sum;
	
	sum = calcuCheckSum(mIP.buf + *offset, 20); /* IPֻ����ͷ�� */
	if(sum != 0) return; /* У�鲻ͨ��,�����˰� */

	IP_analyzeHead(&ip, offset); /* ����IP�̶���20byteͷ */
	if(ARP_checkCache(ip.srcAdd, mIP.macTmp) == 0) { /* �������Ŀͻ��˵�IP��MAC�Ƿ���ARP�����У����������׷�� */
		ARP_addCache(ip.srcAdd, mIP.macTmp);
	}
	
	if(bufMatch(ip.dstAdd, mIP.ip, 4) == 0) { /* ���Ŀ���ַ���Ǳ���IP������ */
		return;
	}
	bufCopy(mIP.ipTmp, ip.srcAdd, 4); /* ��ʱ�洢�ͻ���IP */ 
	
	if(ip.protocol == IP_PROTOCOL_ICMP) { /* �����ICMPЭ��(һ�������ping����) */
		ICMP_process(&ip, offset);
	}
	if((ip.protocol == IP_PROTOCOL_UDP) && (mIP.enFlag & ENFLAG_UDP)) {  /* UDPЭ�� */
		UDP_process(&ip, offset);
	}
	if((ip.protocol == IP_PROTOCOL_TCP) && (mIP.enFlag & ENFLAG_TCP)) {  /* TCPЭ�� */
		TCP_process(&ip, offset);
	}
}

