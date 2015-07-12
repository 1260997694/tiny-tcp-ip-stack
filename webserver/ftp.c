#include "ftp.h"
#include "ftp_ack.h"
#include "myTCPIP.h"
#include <stdio.h>
#include <string.h>
#include "ff.h"

extern myTCPIPStruct mIP;

#define FTP_DATA_PORT 0x1414

extern FATFS   fs;
extern FIL     file;
extern DIR     dir;
extern FILINFO fileInfo;

static const char ftpUser[] = "root\r\n";           /* �û��� */
static const char ftpPass[] = "stm32webserver\r\n"; /* ���� */
static const char ftpPath[] = "/";					/* Ŀ¼ */
static char       ftpPathTmp[255] = "/";            /* ��ʱĿ¼ */
static char       ftpPathTmp_BAK[255];              /* ���� */ 
static char       ftpPathBuf[1024 * 2];             /* ����Ŀ¼�µ������ļ���Ϣ */
static char       ftpDownBuf[255];                  /* ��Ҫ���ص��ļ��� */
static TCPInfoStruct infoStruct_BAK;

//#define FTP_DATA_PORT 0x1414  ����������ftp.h��
static const char ftpPasv[] = "(169,254,228,100,20,20).\r\n";

static UINT8 resBuf[260]; 
static UINT8 tinyBuf[80];                          /* ��Ӧ�Ļ����� */
static UINT8 pathBuf[255];

UINT8 ftpStatus = FTP_STU_IDLE;
UINT8 ftpStatus_BAK = FTP_STU_IDLE;
UINT8 ftpMatchUserStu = 0;
UINT8 ftpDataFlag = 0;
UINT8 ftpFirst    = 0;
#define FTP_DATA_FLAG_DIRLIST  1
#define FTP_DATA_FLAG_DOWNLOAD 2
#define FTP_DATA_FLAG_UPLOAD   3

static UINT8 uploadFin = 0; /* ȥ���ϴ����������ر����ӵ����� */


void statusBackup(void);
void statusRestote(void);
mIPErr TCP_saveFile(TCPInfoStruct *info);

mIPErr ftpResponse(UINT16 res, UINT8 *data, UINT8 dataLen, TCPInfoStruct *info, UINT8 enEOF)
{
	mIPErr miperr;
	UINT32 len;

	if(res != 0)
		sprintf((char *)resBuf, "%d ", res);
	else
		resBuf[0] = '\0';
		
	len = strlen((char *)resBuf) + dataLen;	
	bufCopy(resBuf + strlen((char *)resBuf), data, dataLen);
	resBuf[len] = '\0';
	
	if(enEOF) strcat((char *)resBuf, "\r\n");

//	miperr = TCP_replyAndWaitAck(resBuf, strlen((char *)resBuf), info);
	miperr = TCP_reply(resBuf, strlen((char *)resBuf), info);
//	TCP_reply(resBuf, 0, info); /* ACK */

/////////////
	info->ackNumRcv += dataLen + 4;
/////////////	
	
	return miperr;
}


/**
 * @brief ��ȡ��ӦĿ¼�µ������ļ���buf��
 */
void mkDirRsp(char *path, char *buf)
{
	char sizeBuf[10];

	f_opendir(&dir, ftpPathTmp);
	*buf = '\0';
	while(1) {
		f_readdir(&dir, &fileInfo);
		if(fileInfo.fname[0] == 0) break; /* ��û���ļ��ˣ��˳� */
		
		if(fileInfo.fattrib & AM_DIR) strcat(buf, "d");   /* Ŀ¼Ϊ'd'����ͨ�ļ�Ϊ'-' */
		else                          strcat(buf, "-");
			
		strcat(buf, "rwxrwxrwx    2 0        0            "); /* ͨ�� */
		sprintf(sizeBuf, "%d ", (int)fileInfo.fsize);
		strcat(buf, sizeBuf);                                 /* �ļ���С */
		
		if(fileInfo.lfname[0]) strcat(buf, fileInfo.lfname);  /* ���ļ��� */
		else                   strcat(buf, fileInfo.fname);
		
		strcat(buf, "\r\n");
	}
}

mIPErr TCP_sendDataFast(UINT32 mss, UINT32 size, TCPInfoStruct *info);
/**
 * @brief ����ͨ�� 
 */
void ftpDatTsmit(TCPInfoStruct *info)
{
	mIPErr res;
	UINT32 mss, size;
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	switch(ftpDataFlag) {
		case FTP_DATA_FLAG_DIRLIST:
			
			mkDirRsp(ftpPathTmp, ftpPathBuf);			
			ftpResponse(150, (UINT8 *)FTP_ACK_150, strlen(FTP_ACK_150), p, 0);
		
			TCP_replyAndWaitAck((UINT8 *)ftpPathBuf, strlen(ftpPathBuf), info);
			
	
			TCP_send(
				info->srcIP, info->dstPort, info->srcPort,
				info->ackNumRcv, info->squNumRcv,
				20/4,
				TCP_FLAG_ACK | TCP_FLAG_FIN,
				TCP_WINDOW_MAXSIZE,
				0,
				(UINT8 *)tinyBuf, (UINT8 *)tinyBuf, 0
			);
			TCP_waitAck(info, 1);

			ftpResponse(226, (UINT8 *)FTP_ACK_226, strlen(FTP_ACK_226), p, 0);
			
			/* ����Ϊ��ȥ��upload���������ر����ӵ����� */
			if(uploadFin == 1) {
				uploadFin = 0;
				TCP_coonClose(p);
				statusRestote(); /* �ָ� */
			}
			break;
			
		case FTP_DATA_FLAG_DOWNLOAD:
			strcpy(ftpPathTmp_BAK, ftpPathTmp);

printf("DownLoad: %s\r\n", ftpDownBuf);
			ftpResponse(150, (UINT8 *)FTP_ACK_150_D, strlen(FTP_ACK_150_D), p, 0);
		
			mss = (TCP_OPTION_MSS > info->clientMSS) ? (info->clientMSS) : (TCP_OPTION_MSS); /* ����һ���ܷ�������������� */
			res = TCP_vndOpen((UINT8 *)ftpDownBuf, &size, METHOD_FATFS); /* �����ж��������ҳ���Ƿ���� */
			
			if(res != mIP_OK) return;
			
			res = TCP_sendDataFast(mss, size, info);	
			
			if(res == mIP_NOACK) { /* ����Է�����Ӧ�������ر����� */
				TCP_coonClose(info);
			}
			else if(res == mIP_FIN) { /* ����Է������ر����� */
				TCP_coonCloseAck(info);
			}
			else if(res == mIP_RST) { /* ����Է�����RST�������ͷ�������Դ */
				TCP_coonRelease(info);
			}
			
			TCP_vndClose(METHOD_FATFS); /* �ر��ļ�ָ�� */
		
			ftpResponse(226, (UINT8 *)FTP_ACK_226_D, strlen(FTP_ACK_226_D), p, 0);
TCP_waitAck(p, 1);
			TCP_coonClose(p);
			
			statusRestote(); /* �ָ� */
			
printf("RET*****Client: %d -- Server: %d\r\n", p->srcPort, p->dstPort);
tcpCoonOutput();
printf("Status: %d", ftpStatus);			
			
			break;
			
		case FTP_DATA_FLAG_UPLOAD:
			strcpy(ftpPathTmp_BAK, ftpPathTmp);
printf("upload\r\n");
			res = TCP_vndWriteReady((UINT8 *)ftpDownBuf, METHOD_FATFS);
			if(res != mIP_OK) return;
			
			ftpResponse(150, (UINT8 *)FTP_ACK_150_D, strlen(FTP_ACK_150_D), p, 0); /* ��ʼ�������� */
			res = TCP_saveFile(info); /* �����ļ� */

			if(res == mIP_NOACK) { /* ����Է�����Ӧ�������ر����� */
				TCP_coonClose(info);
			}
			else if(res == mIP_FIN) { /* ����Է������ر����� */
				TCP_coonCloseAck(info);
			}
			else if(res == mIP_RST) { /* ����Է�����RST�������ͷ�������Դ */
				TCP_coonRelease(info);
			}
			TCP_vndClose(METHOD_FATFS); /* �ر��ļ�ָ�� */
printf("ok\r\n");
			
			ftpResponse(226, (UINT8 *)FTP_ACK_226_D, strlen(FTP_ACK_226_D), p, 0);
TCP_waitAck(p, 1);
			
			uploadFin = 1;
//			TCP_coonClose(p);
			
//			statusRestote(); /* �ָ� */
			
			break;
			
		default: 
			ftpResponse(220, (UINT8 *)FTP_ACK_220, strlen(FTP_ACK_220), info, 0);
			break;
	}
}


/**
 * @brief �������ڶ���FTPʱ�Ļص����� 
 */
void coonSynCallBack(void)
{
	unsigned int i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		if(mIP.tcpTask->info[i].dstPort == 21 || 1) { /////////////////////////////////////////////////////////
			mIP.tcpTask->state[i] = TCP_COON_CLOSED;
			if(mIP.tcpTask->info[i].dstPort == 21) {
				ftpStatus   = FTP_STU_IDLE;
				ftpDataFlag = FTP_DATA_FLAG_DOWNLOAD;
			}
		}
	}
}


/**
 * @brief FTP���� 
 */
void ftpProcess(TCPInfoStruct *info)
{
	mIPErr res;
	UINT8 tmp;
	UINT32 i;
	char tmpBuf[255];
	
	if(ftpFirst == 0) {
		statusBackup(); /* ���� */
		printf("Client: %d -- Server: %d\r\n", infoStruct_BAK.srcPort, infoStruct_BAK.dstPort);
	}
//	printf("Client: %d -- Server: %d\r\n", infoStruct_BAK.srcPort, infoStruct_BAK.dstPort);
	
	ftpFirst = 1;
	
	switch(ftpStatus) {
		/* ���ӽ�����֮���Ӧ220 */
		case FTP_STU_IDLE:
			ftpResponse(220, (UINT8 *)FTP_ACK_220, strlen(FTP_ACK_220), info, 0);
			ftpStatus = FTP_STU_MATCHNAME;
			goto QUIT;
			
		/* ƥ���û��� */
		case FTP_STU_MATCHNAME:       
			ftpMatchUserStu = 0;
			if(bufMatch(info->data, (UINT8 *)"USER", 4) == 0) {
				TCP_reply(resBuf, 0, info);
				ftpResponse(503, (UINT8 *)FTP_ACK_503, strlen(FTP_ACK_503), info, 0);
				goto QUIT;
			}
			if(bufMatch(info->data + 5, (UINT8 *)ftpUser, strlen(ftpUser))) {
				ftpMatchUserStu = 1; /* �û���ƥ��ɹ� */
			}
			TCP_reply(resBuf, 0, info);
			ftpResponse(331, (UINT8 *)FTP_ACK_331, strlen(FTP_ACK_331), info, 0);
			ftpStatus = FTP_STU_MATCHPASS;
			goto QUIT;
			
		/* ƥ������ */
		case FTP_STU_MATCHPASS:
			if(bufMatch(info->data, (UINT8 *)"PASS", 4)) {
				if(bufMatch(info->data + 5, (UINT8 *)ftpPass, 4)) { /* ��½�ɹ� */
			        ftpStatus = FTP_STU_LOGIN;
					TCP_reply(resBuf, 0, info);
					strcpy(ftpPathTmp, ftpPath);                    /* copy��Ŀ¼�������� */      
					ftpResponse(230, (UINT8 *)FTP_ACK_230, strlen(FTP_ACK_230), info, 0);
					goto QUIT;
				}
			}
			TCP_reply(resBuf, 0, info);
			ftpResponse(530, (UINT8 *)FTP_ACK_530, strlen(FTP_ACK_530), info, 0);	
			goto QUIT;
			
		/* ��½�ɹ� */
		case FTP_STU_LOGIN:
			if(bufMatch(info->data, (UINT8 *)"SYST", 4)) {  
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(215, (UINT8 *)FTP_SYST, strlen(FTP_SYST), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"FEAT", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(0, (UINT8 *)FTP_ACK_211_START, strlen(FTP_ACK_211_START), info, 0);
				TCP_waitAck(info, 1);
				for(tmp = 0; tmp < FTP_ACK_211_NUM; tmp++) {
					res = ftpResponse(0, (UINT8 *)FTP_ACK_211[tmp], 7, info, 0);
					TCP_waitAck(info, 1);
				}
				res = ftpResponse(0, (UINT8 *)FTP_ACK_211_END, strlen(FTP_ACK_211_END), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"OPTS", 4)) {
				if(bufMatch(info->data + 5, (UINT8 *)"UTF8 ON", 7)) {
					ftpResponse(200, (UINT8 *)FTP_ACK_200_U, strlen(FTP_ACK_200_U), info, 0);
				}
			}
			else if(bufMatch(info->data, (UINT8 *)"TYPE", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(200, (UINT8 *)FTP_ACK_200, strlen(FTP_ACK_200), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"PASV", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				bufCopy(tinyBuf, (UINT8 *)FTP_ACK_227, strlen(FTP_ACK_227) + 1);
				strcat((char *)tinyBuf, ftpPasv);
				res = ftpResponse(227, (UINT8 *)tinyBuf, strlen((char *)tinyBuf), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"LIST", 4)) {
				ftpDataFlag = FTP_DATA_FLAG_DIRLIST;
			}
			else if(bufMatch(info->data, (UINT8 *)"CDUP", 4)) {
				strcpy(ftpPathTmp, ftpPath);
				TCP_reply(resBuf, 0, info); /* ACK */
				ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"PWD", 3)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				strcpy(tmpBuf, "\"");
				strcat(tmpBuf, ftpPathTmp);
				strcat(tmpBuf, "\"\r\n");
				ftpResponse(257, (UINT8 *)tmpBuf, strlen(tmpBuf), info, 0);
printf("PWD: %s\r\n", ftpPathTmp);			
			}
			else if(bufMatch(info->data, (UINT8 *)"RETR", 4)) {
printf("RETR: %s\r\n", info->data + 5);
				ftpDataFlag = FTP_DATA_FLAG_DOWNLOAD;
				strcpy(ftpDownBuf, ftpPathTmp);                /* ����Ŀ¼ */
				strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)info->data + 5);    /* �����ļ��� */
				for(i = 0; i < 2000; i++) { /* delete "\r\n" */
					if(*(ftpDownBuf + i) == '\r') {
						*(ftpDownBuf + i) = '\0';
						break;
					}
				}
			}
			else if(bufMatch(info->data, (UINT8 *)"CWD", 3)) {
				if(*(info->data + 4) == '/') { /* ����Ǿ���Ŀ¼ */
					strcpy(ftpPathTmp, (char *)info->data + 4);
				}
				else { /* ���Ŀ¼ */
					if(strlen(ftpPathTmp) != 1) /* ���Ȳ�Ϊ1˵������rootĿ¼ */
						strcat(ftpPathTmp, "/");
					strcat(ftpPathTmp, (char *)info->data + 4);
				}
				for(i = 0; i < 2000; i++) { /* delete "\r\n" */
					if(*(ftpPathTmp + i) == '\r') {
						*(ftpPathTmp + i) = '\0';
						break;
					}
				}
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
				
printf("CWD: %s\r\n", ftpPathTmp);
			}
			
			else if(bufMatch(info->data, (UINT8 *)"STOR", 4)) { /* �ϴ��ļ� */
				ftpDataFlag = FTP_DATA_FLAG_UPLOAD;
				
				strcpy(ftpDownBuf, ftpPathTmp);
				if(strlen(ftpPathTmp) != 1) /* ���Ȳ�Ϊ1˵������rootĿ¼ */
					strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)(info->data + 5));
				
printf("STOR: %s\r\n", ftpDownBuf);
			}
			
			else if(bufMatch(info->data, (UINT8 *)"DELE", 4)) { /* ɾ���ļ� */
				TCP_reply(resBuf, 0, info); /* ACK */
				
				strcpy(ftpDownBuf, ftpPathTmp);
				if(strlen(ftpPathTmp) != 1) /* ���Ȳ�Ϊ1˵������rootĿ¼ */
					strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)(info->data + 5));
				
//				f_unlink(ftpDownBuf); /* ɾ���ļ� */
				TCP_vndClose(METHOD_FATFS);
printf("delete file: %s: -%x\r\n", ftpDownBuf,  f_unlink(ftpDownBuf));				
				ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
			}
			
			goto QUIT;
			
		default: break;
	}
	
	QUIT: tmp = tmp;
}

/**
 * @brief ����������Ϣ
 */
void statusBackup(void)
{
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	strcpy(ftpPathTmp_BAK, ftpPathTmp);
	ftpStatus_BAK = ftpStatus;
	bufCopy((UINT8 *)&infoStruct_BAK, (UINT8 *)p, sizeof(infoStruct_BAK));
}

/**
 * @brief �ָ�������Ϣ
 */
void statusRestote(void)
{
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	mIP.tcpTask->state[0] = TCP_COON_ESTAB; /* ������ */
	strcpy(ftpPathTmp, ftpPathTmp_BAK);
//	ftpStatus = ftpStatus_BAK;
	ftpStatus = FTP_STU_LOGIN;
	bufCopy((UINT8 *)p, (UINT8 *)&infoStruct_BAK, sizeof(infoStruct_BAK));
}


/**
 * @brief ��ȡ�ϴ��ļ�������SD��
 */
mIPErr TCP_saveFile(TCPInfoStruct *info)
{
	mIPErr ack;
	UINT32 first = 1;
	UINT32 i, squNum, ackNum;
	UINT32 squNumSnd, ackNumSnd, writeEn;
	UINT8  mssOption[4] = {0x02, 0x04, 0x05, 0xb4}; /* MSS: 1460 */

	UINT8  buf[1500];
	
	while(1) {
		ack = TCP_waitAck(info, 0);       /* ���ͻ����Ƿ񷢳������� */
		if(ack == mIP_OK) {
			squNum = info->squNumRcv;     /* ��¼squ */
			ackNum = info->ackNumRcv;     /* ��¼ack */
			if(first == 1) {              /* �״���Ҫ��¼���ǲ��ж� */
				first  = 0;               /* ����״α�־ */
				writeEn = 1;              /* ʹ��д�� */
				squNumSnd = ackNum;       /* �������Ϊ�Է������� */
				ackNumSnd = squNum + info->dataLen; /* ����������Ϊ�Է����кż������ݳ��� */
//printf("squ: %X -- ack: %X\r\n", squNumSnd, ackNumSnd);
			}
			else {                        /* ��������״���Ҫ��֤�ļ������� */
				if(squNum != ackNumSnd) { /* ���Ŀǰ���յ���������Ϊ�������ⲻ�������� */
					writeEn = 0;          /* ���дʹ�ܱ�־ */
				}
				else {                    /* ��������������� */
					squNumSnd = ackNum;   /* �������Ϊ�Է������� */
					ackNumSnd = squNum + info->dataLen; /* ����������Ϊ�Է����кż������ݳ��� */
					writeEn = 1;          /* ʹ��д�� */
				}
			}
			
			if(writeEn) {                 /* ֻ��д��־Ϊ1��д��SD�� */
				bufCopy(buf, info->data, info->dataLen);        /* ����data */
				TCP_vndWrite(buf, info->dataLen, METHOD_FATFS); /* д�뵽SD�� */
			}			
			
			TCP_send(info->srcIP,         /* Ŀ��IPΪԴIP */
				 info->dstPort,           /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,           /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 squNumSnd,               /* ��� */
				 ackNumSnd,               /* ȷ�Ϻ� */
				 20 / 4,                  /* TCP�ײ����ȣ����е�20�ֽ� */
				 TCP_FLAG_ACK,            /* ��Ӧ */
				 TCP_WINDOW_MAXSIZE,      /* �����Ժ���Ҫ�޸� */
				 0,                       /* ����ָ�� */
				 mssOption,               /* MSS */
				 tinyBuf,                 /* ����ָ�� */
				 0                        /* ���ݳ��� */
				 );
		}
		else if(ack == mIP_RST) {
			return mIP_RST;
		}
		else if(ack == mIP_FIN) {
			if(info->dataLen != 0) {
				bufCopy(buf, info->data, info->dataLen);        /* ����data */
				TCP_vndWrite(buf, info->dataLen, METHOD_FATFS); /* д�뵽SD�� */
			}
			return mIP_FIN;
		}
	}
	return mIP_OK;
}

