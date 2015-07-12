#include "share.h"

/**
 * @brief  ����src�е����ݵ�dst��
 * @param  src: Դָ��
 * @param  dst: Ŀ��ָ��
 * @param  len: ���Ƶ����ݵĳ���
 * @retval none
 */
void bufCopy(UINT8 *dst, UINT8 *src, UINT32 len) 
{	
	if(len == 0) return;

	while(len--) {
		*dst++ = *src++;
	}
}

/**
 * @brief  ��������buf�������Ƿ���ȫ��ͬ
 * @param  buf1: buf1��ָ��
 * @param  buf2: buf2��ָ��
 * @param  len: ��������ݵĳ���
 * @retval 0: ��ƥ�䣬 1��ƥ��
 */
UINT8 bufMatch(UINT8 *buf1, UINT8 *buf2, UINT32 len)
{
	while(len--) {
		if(*buf1++ != *buf2++) return 0;
	}
	
	return 1;
}

/**
 * @brief  ��str�г�ȡǰlen���ַ�Ȼ���������buf���״γ��ֵ�λ��
 * @param  buf: Ŀ���ָ��
 * @param  len1: Ŀ�����󳤶� 
 * @param  str: ����Դָ��
 * @param  len2: strƥ����Ч����
 * @retval 0: �Ҳ�����Ӧƥ�䣬 !0��ƥ���ַ����ֵ�λ�ã�ע���Ǵ�1��ʼ��
 */
UINT32 strPos(UINT8 *buf, UINT32 len1, UINT8 *str, UINT32 len2)
{
	UINT32 i;

	for(i = 0; i < len1; i++) {
		if(bufMatch(buf + i, str, len2)) return i + 1;
	}
	
	return 0;
}

/**
 * @brief  ����У���
 * @param  ptr: ��Ҫ����У��͵�bufferָ��
 * @param  len: ��Ҫ�������ݲ��ֵĳ��ȣ���λΪbyte�����lenΪ�����Ļ������ʱ����Զ���β�����һ��0x00
 * @retval У���
 */
UINT16 calcuCheckSum(UINT8 *ptr, UINT32 len)
{
	UINT32 sum = 0;

	while(len > 1) {
		sum += ((UINT16)*ptr << 8) | (UINT16)*(ptr + 1);
		ptr += 2;
		len -= 2;	
	}
	if(len) { /* ���������������Ҫβ����0 */
		sum += ((UINT16)*ptr << 8) | 0x00;
	}
	while(sum & 0xFFFF0000) {
		sum = (sum & 0x0000FFFF) + (sum >> 16);
	}
	
	return (UINT16)(~sum); /* ���غ͵ķ��� */
}

/**
 * @brief  ͨ������buffer����У���(��Ҫ����UDP��TCPα�ײ���ʵ���ײ�&���ݵ�У��)
 * @param  buf1: buffer1��ָ��
 * @param  len1: buffer1��Ҫ�������ݲ��ֵĳ��ȣ���λΪbyte
 * @param  buf2: buffer2��ָ��
 * @param  len2: buffer2��Ҫ�������ݲ��ֵĳ��ȣ���λΪbyte
 * @retval У���
 */
UINT16 calcuCheckSum2Buf(UINT8 *buf1, UINT32 len1, UINT8 *buf2, UINT32 len2)
{
	UINT32 sum = 0;	

	sum += (UINT16)~calcuCheckSum(buf1, len1);
	sum += (UINT16)~calcuCheckSum(buf2, len2);
	while(sum & 0xFFFF0000) {
		sum = (sum & 0x0000FFFF) + (sum >> 16);
	}
	
	return ~sum;
}

