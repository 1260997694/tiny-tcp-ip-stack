#include "UDP.h"
#include "myTCPIP.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ���
 */
extern myTCPIPStruct mIP;


/**
 * @brief  ����UDPͷ
 * @param  ptr: UDP�ײ��ṹ��ָ�룬������Ž���������
 * @param  offset: offset: buffer��ƫ����ָ��
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
static mIPErr UDP_analyzeHead(UDPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	ptr->srcPort  = MKUINT16BIG(buf);
	buf += 2;
	ptr->dstPort  = MKUINT16BIG(buf);
	buf += 2;
	ptr->length   = MKUINT16BIG(buf);
	buf += 2;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	
	*offset = buf - mIP.buf;
	
	return mIP_OK;
}

/**
 * @brief  ͨ��UDPα�ײ��ṹ������UDPα�ײ���buf��
 * @param  psPtr: UDPα�ײ��ṹ��
 * @param  buf: ��Ž�����α�ײ���buffer
 * @retval none
 */
static void UDP_makePsHead(UDPPseudoHeadStruct *psPtr, UINT8 *buf)
{
	bufCopy(buf, psPtr->srcIP, 4);
	buf += 4;
	bufCopy(buf, psPtr->dstIP, 4);
	buf += 4;
	*buf++ = psPtr->zero;                
	*buf++ = psPtr->protocol;
	UINT16TOPTRBIG(buf, psPtr->length);
}

/**
 * @brief  ͨ��UDP�ײ��ṹ������UDP�ײ���buffer��(ʵ���ϳ����ļ��ײ���������),α�ײ�������buffer����Ҫ����У��͵ļ���
 * @param  ptr: UDP�ײ�ָ��
 * @param  psPtr: UDPα�ײ�ָ��
 * @param  offset: bufferƫ����ָ��
 * @param  data: ���͵�����ָ��
 * @param  dataLen: ���͵����ݴ�С
 * @retval none
 */
static void UDP_makeHead(UDPHeadStruct *ptr, UDPPseudoHeadStruct *psPtr, UINT32 *offset, UINT8 *data, UINT32 dataLen)
{
	UINT8  *buf = mIP.buf + *offset;
	UINT8  psBuf[12];                /* 12�ֽڵ�α�ײ� */
	UINT16 sum;                      /* У��� */
	
	UDP_makePsHead(psPtr, psBuf);    /* ����UDPα�ײ�,����psBuf */
	
	/* ����UDP�ײ� */
	UINT16TOPTRBIG(buf, ptr->srcPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->dstPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->length);
	buf += 2;
	UINT16TOPTRBIG(buf , 0x0000); /* ���Ȱ�У�������Ϊ0 */	
	buf += 2;
	bufCopy(buf, data, dataLen);  /* װ������ */
	buf += dataLen;
	
    sum = calcuCheckSum2Buf(psBuf, 12, mIP.buf + *offset, ptr->length); /* ����У��� */
	UINT16TOPTRBIG(mIP.buf + *offset + 6, sum);     /* д��У��� */
	
	*offset = buf - mIP.buf;
}

/**
 * @brief  ʹ��UDPЭ�鷢������
 * @param  srcPort: Դ�˿ڣ�����������UDP�Ķ˿�
 * @param  dstIP: Ŀ��IP
 * @param  dstPort: Ŀ��˿�
 * @param  data: ���͵����ݵ�ָ��
 * @param  dataLen: ���͵����ݵĳ���
 * @retval mIP_OK: ���ͳɹ�, mIP_ERROR: ����ʧ��
 */
mIPErr UDP_send(UINT16 srcPort, UINT8 *dstIP, UINT16 dstPort, UINT8 *data, UINT32 dataLen)
{
	UDPHeadStruct udp;
	UDPPseudoHeadStruct udpPs;
	ETHHeadStruct eth;
	UINT32 offset;
	UINT8 macTmp[6];
	
	/* ����UDP�ײ� */
	udp.srcPort  = srcPort;
	udp.dstPort  = dstPort;
	udp.length   = dataLen + 8;
	udp.checkSum = 0x0000;
	
	/* ����UDPα�ײ� */
	bufCopy(udpPs.srcIP, mIP.ip, 4);
	bufCopy(udpPs.dstIP, dstIP, 4);
	udpPs.zero = 0x00;
	udpPs.protocol = IP_PROTOCOL_UDP;
	udpPs.length = dataLen + 8;
	
	/* ��������ǰ��ȡIP��Ӧ��MAC��ַ�������ȡ��������ERROR */
	if(ARP_getMacByIp(mIP.ipTmp, macTmp) != mIP_OK) return mIP_NOACK;
	
	/* ƫ�������㣬��ʼ�������� */
	offset = 0;
	/* д��ETHͷ */
	bufCopy(eth.dstAdd, macTmp, 6);
	bufCopy(eth.srcAdd, mIP.mac, 6);
	eth.type = PROTOCOL_TYPE_IP;
	ETH_makeHead(&eth, &offset);
	/* д��IPͷ */
	IP_makeHeadDefault(udp.length, mIP.identification++, 0, 0, IP_PROTOCOL_UDP, &offset);
	/* д��UDPͷ */
	UDP_makeHead(&udp, &udpPs, &offset, data, dataLen);
	/* �������� */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	return mIP_OK;
}

/**
 * @brief  UDP����
 * @param  ip: ����UDP��IP�ײ�ָ�룬��Ҫ��������UDP���ݴ�С��У���
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void UDP_process(IPHeadStruct *ip, UINT32 *offset)
{
	UDPHeadStruct udp;
	UDPPseudoHeadStruct udpPs;
	UINT8 psBuf[12];             /* 12�ֽڵ�α�ײ� */
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;

	/* ����α�ײ� */
	bufCopy(udpPs.srcIP, ip->srcAdd, 4);
	bufCopy(udpPs.dstIP, ip->dstAdd, 4);
	udpPs.zero = 0x00;
	udpPs.protocol = ip->protocol;
	udpPs.length = ip->totalLength - ip->headLength * 4;	
	UDP_makePsHead(&udpPs, psBuf); /* ����UDPα�ײ�,����psBuf */
	
	sum = calcuCheckSum2Buf(psBuf, 12, buf, ip->totalLength - ip->headLength * 4); /* ����У��� */	
	if(sum != 0) return; /* ���У��Ͳ�Ϊ0������ */ 

	UDP_analyzeHead(&udp, offset); /* ����UDP�ײ� */

//////////////////////
	bufCopy(mIP.datBuf, mIP.buf + *offset, udp.length - 8);
	udpCallBack(&udp, mIP.datBuf, udp.length - 8);
}

