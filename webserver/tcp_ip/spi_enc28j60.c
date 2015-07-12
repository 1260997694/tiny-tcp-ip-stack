/******************** (C) COPYRIGHT 2011 ����Ƕ��ʽ���������� ********************
 * �ļ���  ��spi.c
 * ����    ��ENC28J60(��̫��оƬ) SPI�ӿ�Ӧ�ú�����
 *          
 * ʵ��ƽ̨��Ұ��STM32������
 * Ӳ�����ӣ� ------------------------------------
 *           |PB13         ��ENC28J60-INT (û�õ�)|
 *           |PA6-SPI1-MISO��ENC28J60-SO          |
 *           |PA7-SPI1-MOSI��ENC28J60-SI          |
 *           |PA5-SPI1-SCK ��ENC28J60-SCK         |
 *           |PA4-SPI1-NSS ��ENC28J60-CS          |
 *           |PE1          ��ENC28J60-RST (û��)  |
 *            ------------------------------------
 * ��汾  ��ST3.0.0
 * ����    ��fire  QQ: 313303034
 * ����    ��firestm32.blog.chinaunix.net 
**********************************************************************************/
#include "spi_enc28j60.h"
#include "spi.h"

/*
 * ��������SPI1_Init
 * ����  ��ENC28J60 SPI �ӿڳ�ʼ��
 * ����  ���� 
 * ���  ����
 * ����  ����
 */																						  
void SPI_Enc28j60_Init(void)
{
	spiInit();
}

/*
 * ��������SPI1_ReadWrite
 * ����  ��SPI1��дһ�ֽ�����
 * ����  �� 
 * ���  ��
 * ����  ��
 */

unsigned char	SPI1_ReadWrite(unsigned char writedat)
{
	return spiSR(writedat);
}

