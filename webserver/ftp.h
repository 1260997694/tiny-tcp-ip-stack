#ifndef __FTP_H
#define __FTP_H

#include "datatype.h"
#include "tcp.h"

#define FTP_STU_IDLE       0 /* ���� */
#define FTP_STU_MATCHNAME  1  /* ƥ���û��� */
#define FTP_STU_MATCHPASS  2  /* ƥ������ */
#define FTP_STU_LOGIN      3  /* ��½�ɹ� */

#define FTP_DATA_PORT 0x1414  /* ʮ�����Ƶ�14Ϊʮ���Ƶ�20 */

typedef struct FTPTmpStruct {
	UINT32 ctlSquNumRcv;
	UINT32 ctlAckNumRcv;
	UINT32 datSquNumRcv;
	UINT32 datAckNumRcv;
} FTPTmpStruct;

void ftpProcess(TCPInfoStruct *info);
void ftpDataTsmit(TCPInfoStruct *info);

#endif

