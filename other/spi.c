#include "spi.h"
#include "stm32f4xx.h"

#define SPI_MODE_HARDWARE
#define SPI_DELAY() Delay(0xF)
void Delay(unsigned long x);

/**
 * @brief SPI���ų�ʼ��(PA4-CS , PA5-CLK , PA6-MISO , PA7-MOSI)
 */
static void spiPinInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); /* ʹ��GPIOAʱ�� */
#ifdef SPI_MODE_HARDWARE                                 /* Ӳ��SPI */	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;        /* ����ģʽ */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* ������� */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;            /* CS��������ͨIO��ʼ�� */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;       
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	SPI_CS_H;
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);  /* ����PA5������ΪSPI1_CLK */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);  /* ����PA6������ΪSPI1_MISO */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);  /* ����PA7������ΪSPI1_MOSI */

#else  /* ���ģ��SPI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* ������� */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;            /* CS��������ͨIO��ʼ�� */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;       
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	SPI_CS_H;
#endif
}

/**
 * @brief SPI��ʼ��
 */
void spiInit(void)
{
#ifdef SPI_MODE_HARDWARE
	SPI_InitTypeDef spi;

	spiPinInit();  /* ��ʼ��SPI1���� */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); /* ʹ��SPI1ʱ�� */
	
	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex; /* ˫��˫��ȫ˫�� */
	spi.SPI_Mode = SPI_Mode_Master;                      /* ��SPI */
	spi.SPI_DataSize = SPI_DataSize_8b;                  /* 8bit���� */
	spi.SPI_CPOL = SPI_CPOL_Low;                         /* ʱ�ӿ���ʱΪ�͵�ƽ */
	spi.SPI_CPHA = SPI_CPHA_1Edge;                       /* ǰʱ���ز��������CPOL_L��Ϊ�����ز��� */
	spi.SPI_NSS = SPI_NSS_Soft;                          /* �������NSS */
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; /* 2��Ƶ */
	spi.SPI_FirstBit = SPI_FirstBit_MSB;                 /* ��λ��ǰ */
	SPI_Init(SPI1, &spi);
	
	SPI_Cmd(SPI1, ENABLE);                               /* ʹ��SPI1 */

#else
	spiPinInit();  /* ��ʼ��SPI1���� */
	SPI_CS_H;
	SPI_MOSI_H;
	SPI_CLK_L;
#endif
}

/**
 * @brief SPI���Ͳ��ҽ���һ���ֽ�,
 * ��spi.h���������꣺SPI_CS_H and SPI_CS_L,�����������PA4
 * @param data: �������͵�����
 * @retval ���յ��ֽ�
 */
unsigned char spiSR(unsigned char data)
{
#ifdef SPI_MODE_HARDWARE
/*	
    ע�͵���ʹ�ÿ⺯���ķ��ͽ��մ��룬Ч�ʱ�ֱ�Ӳ����Ĵ�����
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);
	SPI_I2S_SendData(SPI1, data);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);
	return SPI_I2S_ReceiveData(SPI1);
 */
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = data;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE)) {}
    return SPI1->DR;
#else
	unsigned char i, tmp = 0;
	
	SPI_CLK_L;
	SPI_DELAY();
	for(i = 0; i < 8; i++) {
		if(data & (0x80 >> i)) SPI_MOSI_H;
		else                   SPI_MOSI_L;
		SPI_DELAY();
		tmp |= (SPI_MISO_IN) ? (0x80 >> i) : (0x00);
		SPI_DELAY();
		SPI_CLK_H;   /* �����ز��� */		
		SPI_DELAY(); 
		SPI_CLK_L;   /* �½��ظı����� */
	}
	
	return tmp;
#endif
}

/**
 * @brief SPI��һ���ֽ�,���ڲ��ܽ����ж������ٶȱ�spiSR��
 * ��spi.h���������꣺SPI_CS_H and SPI_CS_L,�����������PA4
 * @param data: �������͵�����
 */
void spiSend(unsigned char data)
{
#ifdef SPI_MODE_HARDWARE
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = data;
#else

#endif
}


/**
 * @brief SPI����һ���ֽ�,�ٶȺ�spiSRһ��
 * ��spi.h���������꣺SPI_CS_H and SPI_CS_L,�����������PA4
 * @retval ���յ�������
 */
unsigned char spiReceive(void)
{
#ifdef SPI_MODE_HARDWARE
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = 0xFF;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE)) {}
    return SPI1->DR;	
#else

#endif
}

