#include "myTCPIP.h"
#include "share.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ���  
 */
myTCPIPStruct mIP;

/**
 * @brief ������ȫ�ֱ���������ʱ�洢���յ�MAC��ַ  
 */
ETHHeadStruct ETHHead;


/**
 * @brief  ��ʼ��TCP/IPЭ��ջ
 * @param  mac: mac��ַָ��
 * @param  ip: ip��ַָ��
 * @param  buf: buffer(������)ָ�룬�Ժ����ݵķ��ͺͽ��ն���ʹ�������ַ����mIP.bufά��
 * @param  datBuf: data buffer(���ݻ�����)ָ�룬������ʱ��ȡ���ݽ����ϲ㣬��mIP.datBufά��
 * @retval �����־��0Ϊ�ɹ�
 */
mIPErr myTCPIP_init(UINT8 *mac, UINT8 *ip, UINT8 *buf, UINT8 *datBuf)
{
	bufCopy(mIP.mac, mac, 6);
	bufCopy(mIP.ip, ip, 4);
	mIP.buf = buf;
	mIP.datBuf = datBuf;
	mIP.identification = 0x0000;
	mIP.arpCache.num = 0;
	mIP.enFlag = 0;  /* ��־���� */

	return myTCPIP_driverInit(mac);
}

/**
 * @brief  TCP/IP����ӿڣ�û�¾�һֱ���ð�
 * @param  none
 * @retval none
 */
void myTCPIP_process(void)
{
    UINT32 len, offset = 0;
	
	len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
	if(len == 0) return;

	offset = 0; /* ����buf��ƫ����Ϊ0���Ա���ڴ��� */
	ETH_analyzeHead(&ETHHead, &offset);
	bufCopy(mIP.macTmp, ETHHead.srcAdd, 6); /* ��ʱ�洢�ͻ���MAC */
	
	if(ETHHead.type == PROTOCOL_TYPE_ARP) { /* ARPЭ�� */
		ARP_process(&offset);
		return;
	}
	if(ETHHead.type == PROTOCOL_TYPE_IP) { /* IPЭ�� */
		IP_process(&offset);
	}
}

