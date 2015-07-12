#include "driver.h"
#include "enc28j60.h"
#include "stm32f4xx.h"

extern unsigned long sysTime;


/**
 * @brief  ��̫������ʼ��
 * @param  mac: ����MAC��ַ
 * @retval ������ʼ���ɹ���־��0: �ɹ�
 */
mIPErr myTCPIP_driverInit(UINT8 *mac)
{
	enc28j60Init(mac);
	enc28j60PhyWrite(PHLCON,0x476);
	
	return mIP_OK;
}

/**
 * @brief  ��ȡ���ݰ�
 * @param  buf: װ������buf��ָ��
 * @param  macLength: ����ȡ������
 * @retval ��ȡ�������������û���յ������򷵻�0
 */
UINT32 myTCPIP_getPacket(UINT8 *buf, UINT32 maxLength)
{
	return enc28j60PacketReceive(maxLength, buf);
}

/**
 * @brief  �������ݰ�
 * @param  buf: ���ݰ�ָ��
 * @param  length: ����������
 * @retval 0: ���ͳɹ�
 */
mIPErr  myTCPIP_sendPacket(UINT8 *buf, UINT32 length)
{
	enc28j60PacketSend(length, buf);
	while(enc28j60Read(ECON1) & ECON1_TXRTS); /* �ȴ����ݷ�����ϣ���֤TCP���������� */
	return mIP_OK;
}

/**
 * @brief  ��ȡϵͳʱ�䣬ϵͳʱ��������û�����ά������λΪ����
 * @param  none
 * @retval ϵͳʱ�䣬��λΪ����
 */
UINT32 myTCPIP_getTime(void)
{
	return sysTime;
}


