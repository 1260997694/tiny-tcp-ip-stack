#include "usart.h"
#include <stdio.h>
#include "stm32f4xx.h"

/**
 * @brief ��ʼ��uartIO
 */
static void usartPinInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;            /* uart2��TX��PA2 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;        /* ����ģʽ */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* ������� */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  /* ����PA2������Ϊusart2ʹ�� */
}

/**
 * @brief ��ʼ������
 */
void usartInit(void)
{
	USART_InitTypeDef usart;
	
	usartPinInit();  /* ��ʼ��uart2��Ӧ��TX���� */
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);              /* ʹ��uart2��ʱ�� */
	usart.USART_BaudRate = 115200;                                      /* ������Ϊ115200 */
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   /* ʧ��Ӳ�������� */
	usart.USART_Mode = USART_Mode_Tx;                                   /* ������ */
	usart.USART_Parity = USART_Parity_No;                               /* ʧ��У�� */
	usart.USART_StopBits = USART_StopBits_1;                            /* 1λֹͣλ */
	usart.USART_WordLength = USART_WordLength_8b;                       /* 8byte���� */
	
	USART_Init(USART2, &usart);
	USART_Cmd(USART2, ENABLE);   /* ʹ��usart2 */
	
}

/**
 * @brief ����һ���ֽ�
 * @param data: ��Ҫ���͵�����
 */
static void usartSend(unsigned char data)
{
	USART_SendData(USART2, data);    /* �������� */
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);  /* �ȴ�������� */
}

/**
 * @brief printf��������
 */
int fputc(int c, FILE *f)
{
  	usartSend(c);
  	return c;
}
