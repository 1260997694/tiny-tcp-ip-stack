#include "ETH.h"
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
 * @brief  ������̫���ײ�
 * @param  ptr: ��̫���ײ��ṹ��ָ��
 * @param  offset: buffer��ƫ����ָ��
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
mIPErr ETH_analyzeHead(ETHHeadStruct *ptr, UINT32 *offset)
{
	bufCopy(ptr->dstAdd, mIP.buf + *offset, 6);
	*offset += 6;
	bufCopy(ptr->srcAdd, mIP.buf + *offset, 6);
	*offset += 6;
	ptr->type = MKUINT16BIG(mIP.buf + *offset);
    *offset += 2;
	
	return mIP_OK;
}

/**
 * @brief  ͨ����̫���ײ��ṹ��������̫���ײ���buffer��
 * @param  ptr: ��̫���ײ��ṹ��ָ��
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void ETH_makeHead(ETHHeadStruct *ptr, UINT32 *offset)
{
	bufCopy(mIP.buf + *offset, ptr->dstAdd, 6);
    *offset += 6;
	bufCopy(mIP.buf + *offset, ptr->srcAdd, 6);
	*offset += 6;
	*(mIP.buf + *offset) = (UINT8)(ptr->type >> 8);
	*offset += 1;
	*(mIP.buf + *offset) = (UINT8)ptr->type;
	*offset += 1;
}

/**
 * @brief  ����Ĭ����̫���ײ���buffer��,��Ҫ����Ӧ��ͻ���ʱ��ԴMAC��Ŀ��MAC�Ϳͻ��˷�����λ�û����ٷ���
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void ETH_makeHeadDefault(UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	bufCopy(buf, ETHHead.srcAdd, 6);
	buf += 6;
	bufCopy(buf, ETHHead.dstAdd, 6);
	buf += 6;
	UINT16TOPTRBIG(buf, ETHHead.type);
	buf += 2;
	*offset = buf - mIP.buf;
}

