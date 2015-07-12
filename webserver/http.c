#include "http.h"
#include "http_ack.h"
#include "myTCPIP.h"
#include <string.h>
#include "ff.h"
#include <stdio.h>
#include "stm32f4xx.h"
                             
#define CHECK 0
#define WAIT  1							 

extern FATFS fs;
extern FIL  file;

							 
static UINT8 HTTP_STATUS_BUF[200];
static UINT8 sndBuf[1550];
static UINT8 urlBuf[_MAX_LFN];
static UINT8 nameBuf[_MAX_LFN];

unsigned int decodeJson(char *jsonStr, char *attr, char *data);

/** 
 * @brief ������������
 */
#define VirtualHostNum 2 /* ������������ */
static const char VirtualHost[VirtualHostNum * 2][255] = { /* �����������ã���ʽΪ"������","·����" */
	"stm32", "/stm32",                   
	"server", "/server"
};


mIPErr TCP_sendDataFast(UINT32 mss, UINT32 size, TCPInfoStruct *info)
{
	mIPErr ack;
	UINT32 tmpSqu, tmpAck, fstSqu, fstAck, tmp;
	UINT32 time;                     
	UINT32  p1 = 0, p2 = 0, p3 = 0;  /* ���ڵ�����ָ��,��ʾ����ڴ��������ײ���ƫ���� */
	                                 /* p1��ʾ�Ѿ�ȷ�϶Է��յ��˶����ֽڣ�p2��ʾ�ѷ��͵�δ�յ�ȷ�ϣ�p3��ʾ�����ײ� */
	fstSqu = info->squNumRcv;        
	fstAck = info->ackNumRcv;
	tmpSqu = info->squNumRcv;        
	tmpAck = info->ackNumRcv;        /* ��ʱ�洢�ͻ��˵�squ��ack */
	
	if(size <= info->clientWnd) {    /* ������͵����ݴ�СС�ڻ���ڶԷ��Ĵ��ڴ�С,��ôp3ֱ���ƶ����ļ�β�� */
		p3 = size - 1;
	}
	else {
		p3 = info->clientWnd - 1;    
	}
	TCP_vndMvPtr(0, METHOD_FATFS); 

	if(size < mss) { /* ��������൱�٣���ôֱ�ӷ��Ͳ��ȴ�ack������ */
		TCP_vndGet(sndBuf, size, METHOD_FATFS);
		TCP_replyAndWaitAck(sndBuf, size, info);
		TCP_coonClose(info);
	}
	else { /* ������ݺܶ����Ҫģ�⻬�����ڵķ�ʽ�������� */
		time = myTCPIP_getTime();
		while(1) {
			tmp = ((p3 - p2 + 1) > mss) ? (mss) : (p3 - p2 + 1);  /* ���㼴�����͵Ĵ�С�����������Ϊmss */
			if(p2 > p3 || info->clientWnd == 0) tmp = 0;          
			
			TCP_vndMvPtr(p2, METHOD_FATFS);                       /* �ƶ�ָ�뵽P2λ��(�������͵���������λ��) */
			TCP_vndGet(sndBuf, tmp, METHOD_FATFS);		          /* ��ȡ�������͵����� */
			
			
			
			if(tmp != 0) {			
				TCP_send(info->srcIP, /* Ŀ��IPΪԴIP */
				 info->dstPort,       /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,       /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 fstAck + p2,         /* ��ţ�ǰ���Ѿ�������� */
				 tmpSqu,              /* ȷ�Ϻţ�ǰ���Ѿ�������� */
				 20 / 4,              /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
				 TCP_FLAG_ACK,        /* ��Ӧ */
				 TCP_WINDOW_MAXSIZE,  /* �����Ժ���Ҫ�޸� */
				 0,                   /* ����ָ�� */
				 sndBuf,              /* MSS */
				 sndBuf,              /* ����ָ�� */
				 tmp                  /* ���ݳ��� */
				 );
			}		
			p2 += tmp;		                       /* tmp��С���ݷ������ */
		
			ack = TCP_waitAck(info, CHECK);        /* ���ͻ����Ƿ��Ӧ */
			if(ack == mIP_OK) {                                  
				time = myTCPIP_getTime();          /* ��Ӧ�����ʱ�� */
				tmpSqu = info->squNumRcv;          
				tmpAck = info->ackNumRcv;
				p1 = tmpAck - fstAck;
				p3 = p1 + info->clientWnd;
				if(p3 > size - 1) p3 = size - 1;
                /**/
				if(p2 == p3 + 1) p2 = p1;
			}
			else if(ack == mIP_RST) {
				return mIP_RST;
			}
			else if(ack == mIP_FIN) {
				return mIP_FIN;
			}
			if((myTCPIP_getTime() - time > TCP_RETRY_TIME) || (time > myTCPIP_getTime())) {
				return mIP_NOACK; /* �����ʱû����Ӧ�Ļ�����ô����NOACK */
			}
		}
	}
	
	return mIP_OK;
}

void htttpAnalyzeUrl(UINT8 *url, UINT8 *fileName)
{
	UINT32 i;
	UINT32 len;

	if(strPos(url, strlen((char *)url), (UINT8 *)"?", 1))
		len = strPos(url, strlen((char *)url), (UINT8 *)"?", 1) - 1;
	else
		len = strlen((char *)url);

	bufCopy(fileName, url, len);
	*(fileName + len) = '\0';
	
	for(i = len - 1; i > 0; i--) {
		if(*(fileName + i) == '.') {
			break;
		}
	}
	
	if(i == 0) { /* ���û�к�׺�������п������ļ���Ҳ�п�����Ŀ¼������һ�㶼��Ŀ¼ */
		if(bufMatch(url, (UINT8 *)"/", 2)) strcat((char *)fileName, "index.html"); 
		else                               strcat((char *)fileName, "index.html");
	}
}

void httpMakeReply(UINT16 ackNum, UINT8 *fileName, UINT32 cntSize)
{
	UINT32 i;
	
	for(i = strlen((char *)fileName); i > 0; i--) {
		if(*(fileName + i) == '.') {
			break;
		}
	}
	
	switch(ackNum) {
		case 200: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 200 OK\r\n"); break;
		case 400: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 400 Bad Request\r\n"); break;
		case 404: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 404 Not Found\r\n"); break;
		default: break; 
	}
	
	if(bufMatch(fileName + i + 1, (UINT8 *)"html", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: text/html\r\n");
	}
	else if(bufMatch(fileName + i + 1, (UINT8 *)"mp3", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: audio/mpeg\r\n");
	}
	else if(bufMatch(fileName + i + 1, (UINT8 *)"mp4", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: video/mp4\r\n");
	}
	
	sprintf((char *)HTTP_STATUS_BUF + strlen((char *)HTTP_STATUS_BUF),
	        "Content-Length: %u\r\n\r\n", (unsigned int)cntSize);
}

void httpGetHtml(TCPInfoStruct *info)
{
	mIPErr res;
	UINT32 mss, size;
	
	htttpAnalyzeUrl(urlBuf, nameBuf); 
	printf("fileName: %s\r\n", nameBuf);
		
	mss = (TCP_OPTION_MSS > info->clientMSS) ? (info->clientMSS) : (TCP_OPTION_MSS); /* ����һ���ܷ�������������� */
	res = TCP_vndOpen(nameBuf, &size, METHOD_FATFS); /* �����ж��������ҳ���Ƿ���� */

	if(res == mIP_OK) { /* ��������������ļ� */

		httpMakeReply(200, nameBuf, size);
		res = TCP_replyAndWaitAck((UINT8 *)HTTP_STATUS_BUF, strlen((char *)HTTP_STATUS_BUF), info); /* ���ȷ���200���ȴ���Ӧ����Ҫ��Ϊ�˷���֮����ٴ��ļ��Ĳ��� */
//		if(mss > 1024) mss = 1024; /* ���SD�����Ż� */
		res = TCP_sendDataFast(mss, size, info);	
		
		if(res == mIP_NOACK) { /* ����Է�����Ӧ�������ر����� */
			printf("NOACK\r\n");
			TCP_coonClose(info);
		}
		else if(res == mIP_FIN) { /* ����Է������ر����� */
			printf("FIN\r\n");
			TCP_coonCloseAck(info);
		}
		else if(res == mIP_RST) { /* ����Է�����RST�������ͷ�������Դ */
			printf("RST\r\n");
			TCP_coonRelease(info);
		}
	}
	else { /* ��Ȼ����404���� */
		httpMakeReply(404, nameBuf, strlen(HTTP_ACK_404));
		TCP_replyAndWaitAck((UINT8 *)HTTP_STATUS_BUF, strlen((char *)HTTP_STATUS_BUF), info);
		TCP_replyAndWaitAck((UINT8 *)HTTP_ACK_404, strlen(HTTP_ACK_404), info);
		TCP_coonClose(info);
	}
}

void httpGetProcess(TCPInfoStruct *info)
{
	httpGetHtml(info);
}

void httpCallBack(char *url) {
	char dataBuf[20], flag = 0;
	char ledState[3];
	
	flag = decodeJson(url, "led0", dataBuf);
	if(flag == 0) return;
	printf("dataBuf: %s\r\n", dataBuf);
	ledState[0] = (dataBuf[0] == '0') ? (0) : (1);
	
	flag = decodeJson(url, "led1", dataBuf);
	if(flag == 0) return;
	ledState[1] = (dataBuf[0] == '0') ? (0) : (1);
	
	flag = decodeJson(url, "led2", dataBuf);
	if(flag == 0) return;
	ledState[2] = (dataBuf[0] == '0') ? (0) : (1);
	
	printf("flag0: %d, flag1: %d, flag2: %d\r\n", ledState[0], ledState[1], ledState[2]);
	if(ledState[0]) GPIO_SetBits(GPIOC, GPIO_Pin_0);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	if(ledState[1]) GPIO_SetBits(GPIOC, GPIO_Pin_1);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_1);
	if(ledState[2]) GPIO_SetBits(GPIOC, GPIO_Pin_2);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_2);
	
}

void httpProcess(TCPInfoStruct *info)
{
	UINT32 pathLen, i;
	UINT8 *p = info->data;
	
//	printf("info->data: %s\r\n", info->data);
	if(bufMatch(info->data, (UINT8 *)"GET", 3)) { /* ���������GET */
		while(1) {                                /* ��ת����һ�� */
			if(*p == '\r' && *(p + 1) == '\n') {
				p += 2;
				break;
			}
			p++;
		}
		if(bufMatch(p, (UINT8 *)"Host", 4)) {     /* ����host */
			p += 6;
			for(i = 0; i < VirtualHostNum; i++) { /* ���host�Ƿ�������������Ӧ */
				if(bufMatch(p, (UINT8 *)VirtualHost[i * 2], strlen(VirtualHost[i * 2]))) {
					strcpy((char *)urlBuf, VirtualHost[i * 2 + 1]);
					break;
				}
			}
		}
		pathLen = strPos(info->data + 4, _MAX_LFN, (UINT8 *)" ", 1) - 1;
		if(pathLen == 0) return;
		*(info->data + 4 + pathLen) = '\0';
		strcat((char *)urlBuf, (char *)(info->data + 4));
printf("jfdlsaf: %s\r\n", (char *)(info->data + 4));
httpCallBack((char *)(info->data + 4));
		httpGetProcess(info);
	}
	
	TCP_coonReset(); /* ������������ */
}


/**
 * @brief  ����Url��Post�е�json,һά
 * @param  jsonStr: jsonת������ַ���
 * @param  attr: ��Ҫ���ҵ�Ԫ������
 * @param  data: ���Զ�Ӧ������(�ַ���)
 * @retval 0: ƥ��ʧ�� !0: ƥ��ɹ�
 */
unsigned int decodeJson(char *jsonStr, char *attr, char *data) {
    unsigned int i = 0, j = 0;
    /* ƥ�����Ե�λ�ã���1��ʼ */
    unsigned int pos = 0; 
    
	/*  */
	if(strlen(jsonStr) < strlen(attr)) return 0;
	
    /* ����attr��λ�� */
    for(i = 0; i < strlen(jsonStr) - strlen(attr) + 1; i++) {
		for(j = 0; j < strlen(attr); j++) {
			if(*(attr + j) == *(jsonStr + i + j)) continue;
			else break;
		}	
		if(j == strlen(attr)) {
			pos = i + 1;
			break;
		}
    } 

    if(pos == 0) return 0;

    /* �������ص��ַ��� */
    pos += strlen(attr);
    i = 0;
    while(1) {
		*(data + i) = *(jsonStr + pos + i); /* pos֮���Բ���1��Ϊ�պ������˵��ں� */
		i++;
		if((*(jsonStr + pos + i) == '&') || ((*(jsonStr + pos + i) == '\0'))) break;
    }
    *(data + i) = '\0';

    return 1;
}

