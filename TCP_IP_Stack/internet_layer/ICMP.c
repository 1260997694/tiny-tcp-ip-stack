#include "ICMP.h"
#include "myTCPIP.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ��� 
 */
extern myTCPIPStruct mIP;

/**
 * @brief  ����ICMP�ײ�
 * @param  ptr: ICMP�ײ��ṹ��ָ�룬װ�ؽ����������
 * @param  offset: bufferƫ����ָ��
 * @retval 0: �����ɹ�
 */
static mIPErr ICMP_analyzeHead(ICMPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->type = *buf++;
	ptr->code = *buf++;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	if(ptr->type == ICMP_TYPE_REQ) { /* �����ѯ�ʱ��ľͻ�ȡ��ʶ�������к� */
		ptr->identification = MKUINT16BIG(buf);
		buf += 2;
		ptr->sequence = MKUINT16BIG(buf);
		buf += 2;
	}
    ptr->data = buf;
	*offset = buf - mIP.buf;
	
	return mIP_OK;
}

/**
 * @brief  ����ICMP�ײ�(ʵ����Ҳ��������)��buffer�У���Ҫ���ڻ�Ӧ�ͻ��˵�ping
 * @param  ptr: ICMP�ײ��ṹ��ָ��
 * @param  offset: bufferƫ����ָ��
 * @param  data: �跢�͵����ݵ�ָ��
 * @param  dataLen: �跢�͵����ݳ���
 * @param  ip: IP�ײ��ṹ��,�����ſͻ���IP��ַ����Ϣ
 * @retval none
 */
static void ICMP_makeHead(ICMPHeadStruct *ptr, UINT32 *offset, UINT8 *data, UINT32 dataLen, IPHeadStruct *ip)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT16 sum;
	UINT32 headLen = 8;
	
	/* ����ICMPͷ */
	*buf++ = ptr->type;
	*buf++ = ptr->code;
	UINT16TOPTRBIG(buf, 0x0000); /* �Ȱ�У�����Ϊ0 */
	buf += 2;
	if(ptr->type == ICMP_TYPE_REPLY) { /* ����ǻ�Ӧ */
		headLen = 8;
		UINT16TOPTRBIG(buf, ptr->identification);
		buf += 2;
		UINT16TOPTRBIG(buf, ptr->sequence);
		buf += 2;
	}
	bufCopy(buf, data, dataLen); /* �������� */
	sum = calcuCheckSum(mIP.buf + *offset, dataLen + headLen); /* ����У��� */
	UINT16TOPTRBIG(mIP.buf + *offset + 2, sum);
	
	
	*offset = buf - mIP.buf + dataLen;
} 

/**
 * @brief  ICMP����(��Ҫ���ping)
 * @param  ip: �ͻ��˲�ѯʱ��IPͷ���ṹ��
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void ICMP_process(IPHeadStruct *ip, UINT32 *offset)
{
	ICMPHeadStruct icmp;
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;
	
	ICMP_analyzeHead(&icmp, offset);
	if(icmp.type == ICMP_TYPE_REQ) { /* �������Ϊ��ѯ(һ����ping) */
	    sum = calcuCheckSum(buf, ip->totalLength - ip->headLength * 4); /* ICMP���鷶ΧΪͷ��+���� */
		if(sum != 0) return; /* У�鲻ͨ��,���� */

		bufCopy(mIP.datBuf, icmp.data, ip->totalLength - ip->headLength * 4 - 8); /* ��ȡ���� */
		/* ��Ӧ������REQ */
		*offset = 0;
		/* д��ETHͷ */
		ETH_makeHeadDefault(offset);
		/* д��IPͷ */
		IP_makeHeadDefault(ip->totalLength - ip->headLength * 4, mIP.identification++, 0, 0, IP_PROTOCOL_ICMP, offset);
        /* д��ICMPͷ */
		icmp.type = ICMP_TYPE_REPLY;
		icmp.code = ICMP_CODE_REPLY;
		ICMP_makeHead(&icmp, offset, mIP.datBuf, ip->totalLength - ip->headLength * 4 - 8, ip);
		/* �������ݰ� */
		myTCPIP_sendPacket(mIP.buf, *offset);
	}	
}

