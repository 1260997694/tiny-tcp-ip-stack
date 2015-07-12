#include "TCP.h"
#include "myTCPIP.h"

/**
 * @brief ����Э��ջ��ȫ�ֱ���
 */
extern myTCPIPStruct mIP;


void tcpCoonOutput(void);

/**
 * @brief  ����TCPͷ
 * @param  ptr: TCP�ײ��ṹ��ָ�룬������Ž���������
 * @param  offset: offset: buffer��ƫ����ָ��
 * @retval �����־��mIP_OK: �ɹ�, mIP_ERROR: ʧ��
 */
static mIPErr TCP_analyzeHead(TCPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT8 i, tmp;
	
	ptr->srcPort = MKUINT16BIG(buf);
	buf += 2;
	ptr->dstPort = MKUINT16BIG(buf);
	buf += 2;
	ptr->squNum = MKUINT32BIG(buf);
	buf += 4;
	ptr->ackNum = MKUINT32BIG(buf);
	buf += 4;
	ptr->headLen = *buf++ >> 4;
	ptr->flag = *buf++ & 0x3F;
	ptr->window = MKUINT16BIG(buf);
	buf += 2;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	ptr->urgPtr = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->option, buf, ptr->headLen * 4 - 20);
	buf += ptr->headLen * 4 - 20;
	
	ptr->MSS = 536; /* Ĭ��536�ֽ� */
	
	*offset = buf - mIP.buf;
	if(ptr->headLen * 4 - 20 == 0) return mIP_OK;
	
	/* ���½���option(����еĻ�) */
	for(i = 0; i < ptr->headLen * 4 - 20;) {
		if(ptr->option[i] == 1) { /* NOP */
			tmp = 1;
		}
		else {
			tmp = ptr->option[i + 1];
			
			if(ptr->option[i] == 2) { /* MSS */
				if(tmp == 4) {
					ptr->MSS = MKUINT16BIG(ptr->option + i + 2); /* ��ȡMSS */
				}
			}
			if(ptr->option[i] == 3) { /* �������� */
				if(tmp == 3) {
					ptr->window <<= ptr->option[i + 2];
				}
			}
		}
		i += tmp;
	}
	
	return mIP_OK;
}


/**
 * @brief  ͨ��TCPα�ײ��ṹ������TCPα�ײ���buf��
 * @param  psPtr: TCPα�ײ��ṹ��
 * @param  buf: ��Ž�����α�ײ���buffer
 * @retval none
 */
static void TCP_makePsHead(TCPPseudoHeadStruct *psPtr, UINT8 *buf)
{
	bufCopy(buf, psPtr->srcIP, 4);
	buf += 4;
	bufCopy(buf, psPtr->dstIP, 4);
	buf += 4;
	*buf++ = psPtr->zero;                
	*buf++ = psPtr->protocol;
	UINT16TOPTRBIG(buf, psPtr->length);
}


/**
 * @brief  ͨ��TCP�ײ��ṹ������TCP�ײ���buffer��(ʵ���ϳ����ļ��ײ���������),α�ײ�������buffer����Ҫ����У��͵ļ���
 * @param  ptr: TCP�ײ�ָ��
 * @param  psPtr: TCPα�ײ�ָ��
 * @param  offset: bufferƫ����ָ��
 * @param  data: ���͵�����ָ��
 * @param  dataLen: ���͵����ݴ�С
 * @retval none
 */
static void TCP_makeHead(TCPHeadStruct *ptr, TCPPseudoHeadStruct *psPtr, UINT32 *offset, UINT8 *data, UINT32 dataLen)
{
	UINT8  *buf = mIP.buf + *offset;
	UINT8  psBuf[12];                /* 12�ֽڵ�α�ײ� */
	UINT16 sum;                      /* У��� */
	
	TCP_makePsHead(psPtr, psBuf);    /* ����TCP�ײ�,����psBuf */
	
	/* ����TCP�ײ� */
	UINT16TOPTRBIG(buf, ptr->srcPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->dstPort);
	buf += 2;
	UINT32TOPTRBIG(buf, ptr->squNum);
	buf += 4;
	UINT32TOPTRBIG(buf, ptr->ackNum);
	buf += 4;
	*buf++ = ptr->headLen << 4;
	*buf++ = ptr->flag & 0x3F;
	UINT16TOPTRBIG(buf, ptr->window);
	buf += 2;
	UINT16TOPTRBIG(buf, 0x0000); /* ���Ȱ�У�����0 */
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->urgPtr);
	buf += 2;
	bufCopy(buf, ptr->option, ptr->headLen * 4 - 20); /* ����option */
	buf += ptr->headLen * 4 - 20;
	bufCopy(buf, data, dataLen); /* װ������ */
	buf += dataLen;
	
    sum = calcuCheckSum2Buf(psBuf, 12, mIP.buf + *offset, ptr->headLen * 4 + dataLen); /* ����У��� */
	UINT16TOPTRBIG(mIP.buf + *offset + 16, sum);     /* д��У��� */
	
	*offset = buf - mIP.buf;
}

/**
 * @brief  ʹ��TCPЭ�鷢������
 * @param  dstIP: Ŀ��IP
 * @param  srcPort: Դ�˿ڣ�����������TCP�Ķ˿�
 * @param  dstPort: Ŀ��˿�
 * @param  seqNum: ���
 * @param  ackNum: ȷ�Ϻ�
 * @param  headLen: TCP�ײ���С����λΪ4byte
 * @param  flag: ��־
 * @param  window: ����
 * @param  urgPtr: ����ָ��
 * @param  option: ���ȿɱ��ѡ������ݴ�С��Ҫʱ�������headLen��
 * @param  data: ���͵����ݵ�ָ��
 * @param  dataLen: ���͵����ݵĳ���
 * @retval mIP_OK: ���ͳɹ�, mIP_ERROR: ����ʧ��
 */
mIPErr TCP_send(UINT8 *dstIP, UINT16 srcPort, UINT16 dstPort, UINT32 seqNum, UINT32 ackNum, UINT8 headLen, UINT8 flag, UINT16 window, UINT16 urgPtr, UINT8 *option, UINT8 *data, UINT32 dataLen)
{
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	ETHHeadStruct eth;
	UINT32 offset;
	UINT8 macTmp[6];
	
	/* ����TCP�ײ� */
	tcp.srcPort  = srcPort;
	tcp.dstPort  = dstPort;
	tcp.squNum = seqNum;
	tcp.ackNum = ackNum;
	tcp.headLen = headLen;
	tcp.flag = flag;
	tcp.window = window;
	tcp.checkSum = 0x0000;
	tcp.urgPtr = urgPtr;
	bufCopy(tcp.option, option, headLen * 4 - 20);	
	
	/* ����TCPα�ײ� */
	bufCopy(tcpPs.srcIP, mIP.ip, 4);
	bufCopy(tcpPs.dstIP, dstIP, 4);
	tcpPs.zero = 0x00;
	tcpPs.protocol = IP_PROTOCOL_TCP;
	tcpPs.length = dataLen + headLen * 4;
	
	/* ��������ǰ��ȡIP��Ӧ��MAC��ַ�������ȡ��������ERROR */
	if(ARP_getMacByIp(mIP.ipTmp, macTmp) != mIP_OK) return mIP_NOACK;
	
	/* ƫ�������㣬��ʼ�������� */
	offset = 0;
	/* д��ETHͷ */
	bufCopy(eth.dstAdd, macTmp, 6);
	bufCopy(eth.srcAdd, mIP.mac, 6);
	eth.type = PROTOCOL_TYPE_IP;
	ETH_makeHead(&eth, &offset);
	/* д��IPͷ */
	IP_makeHeadDefault(tcp.headLen * 4 + dataLen, mIP.identification++, 0, 0, IP_PROTOCOL_TCP, &offset);
	/* д��TCPͷ */
	TCP_makeHead(&tcp, &tcpPs, &offset, data, dataLen);
	/* �������� */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	return mIP_OK;
}

/**
 * @brief  ͨ��IP+�˿ںŻ�÷ǹر�״̬��TCP���ӵ��ڱ��е�λ��
 * @param  srcIP: Դ(�ͻ���)IP
 * @param  srcPort: Դ(�ͻ���)�˿�
 * @param  dstPort: Ŀ��(����)�˿�
 * @retval 0: ���в����ڴ�IP+�˿ڻ������Ѿ��ر�, !0: ip+�˿��ڱ�(����)�е�λ�ã�����ע��λ���Ǵ�1��ʼ��
 */
UINT32 TCP_getInfoPos(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* �������� */
		if( mIP.tcpTask->state[i] != TCP_COON_CLOSED &&        /* ����û�йر� */
			mIP.tcpTask->info[i].srcPort == srcPort &&         /* Դ�˿�ƥ�� */
			mIP.tcpTask->info[i].dstPort == dstPort &&         /* Ŀ��˿�ƥ�� */
			bufMatch(mIP.tcpTask->info[i].srcIP, srcIP, 4)) {  /* IP��ַƥ�� */
			return (i + 1);
		}
	}
	
	return 0;
}

/**
 * @brief  ͨ��IP+�˿ں����һ��������Ϣ������
 * @param  srcIP: Դ(�ͻ���)IP
 * @param  srcPort: Դ(�ͻ���)�˿�
 * @param  dstPort: Ŀ��(����)�˿�
 * @retval 0: ���ӱ���ȫ��δ�رյ����ӣ������������  !0: ��ӳɹ�,���ҷ�����Ӻ��λ��(��1��ʼ)
 */
static UINT32 TCP_addInfo(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* �������� */
		if(mIP.tcpTask->state[i] == TCP_COON_CLOSED) { /* ֻ���Ѿ��رյ����Ӳ��ܱ����ǵ� */
			bufCopy(mIP.tcpTask->info[i].srcIP, srcIP, 4);
			mIP.tcpTask->info[i].srcPort = srcPort;
			mIP.tcpTask->info[i].dstPort = dstPort;
			return (i + 1);
		}
	}
	
	return 0;
}

/**
 * @brief  TCP����
 * @param  ip: ����TCP��IP�ײ�ָ�룬��Ҫ��������TCP���ݴ�С��У���
 * @param  offset: bufferƫ����ָ��
 * @retval none
 */
void TCP_process(IPHeadStruct *ip, UINT32 *offset)
{
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	UINT8 psBuf[12];             /* 12�ֽڵ�α�ײ� */
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;
	UINT8 option[40];
	UINT32 pos;                  /* �洢�����ڱ��е�λ�� */
	UINT32 tmp;	
	UINT8  mssOption[4] = {0x02, 0x04, 0x04, 0x00}; /* MSS: 1024 */
	
	/* ����α�ײ� */
	bufCopy(tcpPs.srcIP, ip->srcAdd, 4);
	bufCopy(tcpPs.dstIP, ip->dstAdd, 4);
	tcpPs.zero = 0x00;
	tcpPs.protocol = ip->protocol;
	tcpPs.length = ip->totalLength - ip->headLength * 4;	
	TCP_makePsHead(&tcpPs, psBuf); /* ����TCPα�ײ�,����psBuf */
	
	sum = calcuCheckSum2Buf(psBuf, 12, buf, ip->totalLength - ip->headLength * 4); /* ����У��� */	
	if(sum != 0) return; /* ���У��Ͳ�Ϊ0������ */ 

	TCP_analyzeHead(&tcp, offset); /* ����TCP�ײ� */
	
	if((tcp.flag & TCP_FLAG_SYN) && ((tcp.flag & TCP_FLAG_ACK) == 0)) {  /* SYN = 1 && ACK = 0��ʾ�ͻ����������� */
		
		/*------------------------------------*/
		if(tcp.dstPort == TCP_PORT_FTP) {  /* �����FTP�е���ĵģ�21�˿����Ҫ���ػ����⽨��һ�����ӣ����ûص����� */
			coonSynCallBack(); 
		}
		/*------------------------------------*/
				
		if((pos = TCP_getInfoPos(ip->srcAdd, tcp.srcPort, tcp.dstPort)) != 0) { /* ��������ǲ�Ӧ�ó��ֵģ���������˹������������⵼��֮ǰ������û�ص�����ô���ñ�־λΪSYNRCV���� */
			mIP.tcpTask->state[pos - 1] = TCP_COON_SYNRCV; /* ͬ���յ� */
		}
		else { /* ���û�д�����ô�����������ֵ�����Ӧ����ӵ�TCP������Ϣ���� */
			pos = TCP_addInfo(ip->srcAdd, tcp.srcPort, tcp.dstPort); /* ������ӵ����� */
			if(pos == 0) { /* ���û����ӳɹ�����ô�Ͳ���Ӧ */
				return;
			}
			if(pos == 2 && tcp.dstPort == TCP_PORT_HTTP) { /* HTTPֻ�ܽ���һ��TCP���� */
				mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
				return;
			}			
			mIP.tcpTask->info[pos - 1].serverWnd = TCP_WINDOW_MAXSIZE; /* ���÷��������ô���Ϊ�������ֵ */
			mIP.tcpTask->info[pos - 1].clientWnd = tcp.window; /* ���ÿͻ��˿��ô���Ϊ���ֵ */
			mIP.tcpTask->info[pos - 1].clientMSS = tcp.MSS;    /* ���ÿͻ���MSS */
			
			mIP.tcpTask->state[pos - 1] = TCP_COON_SYNRCV;     /* �����ӳɹ������״̬ΪΪSYNRCV */
		}
		TCP_send(mIP.ipTmp, tcp.dstPort, tcp.srcPort, 
			0, tcp.squNum + 1,
			(20 + 4) / 4, TCP_FLAG_SYN | TCP_FLAG_ACK, TCP_WINDOW_MAXSIZE,
			0, mssOption, option, 0); /* ������������������Ӧ */
			
			mIP.tcpTask->info[pos - 1].squNumRcv = tcp.squNum;
			mIP.tcpTask->info[pos - 1].ackNumRcv = tcp.ackNum;
			mIP.tcpTask->info[pos - 1].squNumSnd = 0;
			mIP.tcpTask->info[pos - 1].ackNumSnd = tcp.squNum + 1; /* ���������ݣ�����Ҫ����һ����� */
		return; /* ֱ�ӷ��أ��������������� */
	}
	
	pos = TCP_getInfoPos(ip->srcAdd, tcp.srcPort, tcp.dstPort);
	if(pos == 0) return; /* ʵ������������ʱpos�ǲ����ܵ���0�ģ��������������Ͳ�Ӧ�𣬵ȶԷ��������� */
	
	/* �������ܵ���ע�ͺ���˵��������Ϣ����ȷʵ�ж�Ӧ��������Ϣ */
	
	mIP.tcpTask->info[pos - 1].squNumRcv = tcp.squNum;
	mIP.tcpTask->info[pos - 1].ackNumRcv = tcp.ackNum;
	mIP.tcpTask->info[pos - 1].squNumSnd = tcp.ackNum;
	mIP.tcpTask->info[pos - 1].ackNumSnd = tcp.squNum + ip->totalLength - ip->headLength * 4 - tcp.headLen * 4;
	
	if(mIP.tcpTask->state[pos - 1] == TCP_COON_SYNRCV && tcp.flag & TCP_FLAG_ACK) { /* ���������Ϣ���еı�־λΪSYNRCV����TCP��־ΪACK�Ļ���˵�����ǿͻ��˵ĵ�����������Ӧ��˵��������ȫ���������޸ı�־ΪESTAB */
		mIP.tcpTask->state[pos - 1] = TCP_COON_ESTAB;


#if MYTCPIP_USE_FTP == 1 		
		if(tcp.dstPort == TCP_PORT_FTP) { /* FTP��Ҫ����Ӧ�� */
			mIP.tcpTask->info[pos - 1].dataLen = 0; /* ȥbug */
			
			(*mIP.tcpCb)(&mIP.tcpTask->info[pos - 1]); /* ���ûص����� */
		}	
#endif		
		
		if(tcp.dstPort != TCP_PORT_FTP) {  /* ÿ�ν��������Ӷ�����ûص����� */
		
			coonEstabCallBack(&mIP.tcpTask->info[pos - 1]);
		}
		
		return; /* ���������Ӿ����ȿͻ��˷�������,������������ */
	}
	
	if(tcp.flag & TCP_FLAG_RST) { /* �������RST��־����ô����ر����ӣ�����.... */		
		mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
		return; /* �Ѹ����ӱ��Ϊ�ر�Ȼ������ */
	}

	if(tcp.flag & TCP_FLAG_ACK) { /* TCP�涨�����ӽ������Ժ����д��͵ı��Ķα����ACK��1�������б�Ҫ���һ�� */
		if(tcp.flag & TCP_FLAG_FIN) { /* ����Է���Ҫ�ر����� */
			TCP_coonCloseAck(&mIP.tcpTask->info[pos - 1]);
		}
		else if(mIP.tcpTask->state[pos - 1] == TCP_COON_ESTAB) { /* ��������Ƿ��Ѿ��������� */
			tmp = ip->totalLength - ip->headLength * 4; /* ���ȼ���IPЯ�����ݵĴ�С */
			tmp -= tcp.headLen * 4;                     /* Ȼ�����TCPЯ�����ݵĴ�С */
			
			mIP.tcpTask->info[pos - 1].data = buf + tcp.headLen * 4; /* �洢����ָ�� */
			mIP.tcpTask->info[pos - 1].dataLen = tmp;                /* �洢���ݴ�С */
			if(tmp) (*mIP.tcpCb)(&mIP.tcpTask->info[pos - 1]);       /* ���ûص����� */			
		}
	}
}

/**
 * @brief  �ȴ�or���ͻ��˵���Ӧ
 * @param  info: �ص���������
 * @param  mode: 0:������Ƿ�����Ӧ 1:�ȴ���Ӧ���˳�
 * @retval mIP_OK: �ɹ��� mIP_NOACK: ��ʱ
 */
mIPErr TCP_waitAck(TCPInfoStruct *info, UINT8 mode)
{
	ETHHeadStruct eth;
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	UINT8 psBuf[12];             /* 12�ֽڵ�α�ײ� */
	UINT32 len, offset = 0;
	UINT32 time;  	
	IPHeadStruct ip;
	UINT16 sum;
	UINT8 *buf;
	
	time = myTCPIP_getTime();
	do {
		len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
		if(len == 0) continue;
		
		offset = 0; /* ����buf��ƫ����Ϊ0 */
		ETH_analyzeHead(&eth, &offset);
		
		if(eth.type == PROTOCOL_TYPE_ARP) {   /* ARP */
			ARP_process(&offset);
			return mIP_NOACK;
		}
		else if(eth.type == PROTOCOL_TYPE_IP) { /* IP */
			sum = calcuCheckSum(mIP.buf + offset, 20); /* IPֻ����ͷ�� */
			if(sum != 0) continue; /* У�鲻ͨ��,�����˰� */
			
			IP_analyzeHead(&ip, &offset); /* ����IP�̶���20byteͷ */
			if(bufMatch(ip.dstAdd, mIP.ip, 4) == 0) continue; /* ���Ŀ���ַ���Ǳ���IP������ */
			
			if(ip.protocol == IP_PROTOCOL_TCP) { /* ���㵽��TCP */
				/* ����α�ײ� */
				bufCopy(tcpPs.srcIP, ip.srcAdd, 4);
				bufCopy(tcpPs.dstIP, ip.dstAdd, 4);
				tcpPs.zero = 0x00;
				tcpPs.protocol = ip.protocol;
				tcpPs.length = ip.totalLength - ip.headLength * 4;	
				TCP_makePsHead(&tcpPs, psBuf); /* ����TCPα�ײ�,����psBuf */
				
				buf = mIP.buf + offset;
				sum = calcuCheckSum2Buf(psBuf, 12, buf, ip.totalLength - ip.headLength * 4); /* ����У��� */	
				if(sum != 0) continue; /* ���У��Ͳ�Ϊ0������ */ 
				
				TCP_analyzeHead(&tcp, &offset); /* ����TCP�ײ� */
				
				if(!(tcp.flag & TCP_FLAG_ACK)) continue;
				
				if(tcp.dstPort == info->dstPort && 
				   tcp.srcPort == info->srcPort &&
				   bufMatch(ip.srcAdd, info->srcIP, 4)) {
					
					info->squNumRcv = tcp.squNum;
					info->ackNumRcv = tcp.ackNum;
					info->data      = buf + tcp.headLen * 4;
					info->dataLen   = ip.totalLength - ip.headLength * 4 - tcp.headLen * 4;
					info->clientWnd = tcp.window;
					
					if(tcp.flag & TCP_FLAG_RST) return mIP_RST;      /* RST */
					else if(tcp.flag & TCP_FLAG_FIN) return mIP_FIN; /* FIN */
					
					return mIP_OK;
				}
			}
		} 
	} while( mode && ((myTCPIP_getTime() - time) <= TCP_RETRY_TIME) && (myTCPIP_getTime() >= time) ); /* �����ʱʱ��û������û����� */
	
	return mIP_NOACK;
}

///**
// * @brief  ��λظ���ʼ
// * @param  data: �跢�͵����ݵ�ָ�� 
// * @param  dataLen: �跢�͵����ݵĳ���
// * @param  info: �ص���������
// * @retval mIP_OK: ���ͳɹ��� mIP_ERROR:������������Ϲ淶
// */
//mIPErr TCP_replyMultiStart(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
//{
//	UINT8 optionMSS[4] = {0x02, 0x04};
//	
//	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
//	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

//	info->squNumSnd = info->ackNumRcv;                 /* ���͵�squӦ�õ��ڽ��յ�ack */
//	info->ackNumSnd = info->squNumRcv + info->dataLen; /* ������ackӦ�õ����������͵����+���ݳ��� */

//	TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
//			 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
//			 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
//			 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
//			 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
//			 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
//			 TCP_FLAG_ACK,    /* ��Ӧ */
//			 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
//			 0,               /* ����ָ�� */
//			 optionMSS,       /* MSS */
//			 data,            /* ����ָ�� */
//			 dataLen          /* ���ݳ��� */
//			 ); 
//	info->squNumSnd = info->ackNumRcv + dataLen;

//	return mIP_OK;
//}

///**
// * @brief  �ظ����ǲ��ȴ��ͻ�����Ӧ
// * @param  data: �跢�͵����ݵ�ָ�� 
// * @param  dataLen: �跢�͵����ݵĳ���
// * @param  info: �ص���������
// * @retval mIP_OK: ���ͳɹ��� mIP_ERROR:������������Ϲ淶
// */
//mIPErr TCP_replyMulti(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
//{	
//	UINT8 optionMSS[4] = {0x02, 0x04};
//	
//	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
//	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

//	TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
//			 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
//			 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
//			 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
//			 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
//			 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
//			 TCP_FLAG_ACK,    /* ��Ӧ */
//			 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
//			 0,               /* ����ָ�� */
//			 optionMSS,       /* MSS */
//			 data,            /* ����ָ�� */
//			 dataLen          /* ���ݳ��� */
//			 );        
//    info->squNumSnd = info->squNumSnd + dataLen;                 /* ���͵�squӦ�õ��ڽ��յ�ack */

//	
//	return mIP_OK;
//}

/**
 * @brief  �ظ����ҵȴ��ͻ��˵�Ӧ��,�����ʱ�Զ��ط��������ط��������ܳ���TCP_RETRY_MAXNUM
 * @param  data: �跢�͵����ݵ�ָ�� 
 * @param  dataLen: �跢�͵����ݵĳ���
 * @param  info: �ص���������
 * @retval mIP_OK: ���ͳɹ��� mIP_ERROR:������������Ϲ淶,  mIP_NOACK: �ͻ���һֱ��Ӧ�𲢳���������Դ���
 */
mIPErr TCP_replyAndWaitAck(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
{	
	mIPErr res;
	UINT8 optionMSS[4] = {0x02, 0x04};
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* ������Դ��� */
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

	info->squNumSnd = info->ackNumRcv;                 /* ���͵�squӦ�õ��ڽ��յ�ack */
	info->ackNumSnd = info->squNumRcv + info->dataLen; /* ������ackӦ�õ����������͵����+���ݳ��� */

	do {
		TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
				 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
				 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
				 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
				 TCP_FLAG_ACK,    /* ��Ӧ */
				 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
				 0,               /* ����ָ�� */
				 optionMSS,       /* MSS */
				 data,            /* ����ָ�� */
				 dataLen          /* ���ݳ��� */
				 ); 
		res = TCP_waitAck(info, 1);
		if(res == mIP_FIN || res == mIP_RST) break;
	} while((res == mIP_NOACK) && (--retry)); 
	
//	return (retry) ? (mIP_OK) : (mIP_NOACK);
	return res;
}

/**
 * @brief  �ظ����ǲ���ȴ��ͻ���Ӧ��
 * @param  data: �跢�͵����ݵ�ָ�� 
 * @param  dataLen: �跢�͵����ݵĳ���
 * @param  info: �ص���������
 * @retval mIP_OK: ���ͳɹ��� mIP_ERROR:������������Ϲ淶,  mIP_NOACK: �ͻ���һֱ��Ӧ�𲢳���������Դ���
 */
mIPErr TCP_reply(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
{	
	UINT8 optionMSS[4] = {0x02, 0x04};
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 
	
	info->squNumSnd = info->ackNumRcv;                 /* ���͵�squӦ�õ��ڽ��յ�ack */
	info->ackNumSnd = info->squNumRcv + info->dataLen; /* ������ackӦ�õ����������͵����+���ݳ��� */

	TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
			 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
			 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
			 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
			 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
			 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
			 TCP_FLAG_ACK,    /* ��Ӧ */
			 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
			 0,               /* ����ָ�� */
			 optionMSS,       /* MSS */
			 data,            /* ����ָ�� */
			 dataLen          /* ���ݳ��� */
			 );        
	
	return mIP_OK;
}

/**
 * @brief  �����ر�TCP����
 * @param  info: �ص���������
 * @retval mIP_OK: �ɹ��ر� mIP_NOACK: �ͻ���û��Ӧ�����������ǻ�رյ����ӣ��ͷ���Դ 
 */
mIPErr TCP_coonClose(TCPInfoStruct *info)
{
	UINT8 optionMSS[4] = {0x02, 0x04};
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* ������Դ��� */
	UINT32 pos;
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS); 
	
	info->squNumSnd = info->ackNumRcv; /* ���͵�squӦ�õ��ڽ��յ�ack */
	info->ackNumSnd = info->squNumRcv; /* �ر�֮ǰ�Է��϶���Ӧ��ACK(��������)������������ackӦ�õ����������͵����+0 */
	
	do {
		TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
				 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
				 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
				 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
				 TCP_FLAG_ACK | TCP_FLAG_FIN, /* ��Ӧ&���� */
				 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
				 0,               /* ����ָ�� */
				 optionMSS,       /* MSS */
				 optionMSS,       /* ����ָ�� */
				 0                /* ���ݳ��� */
				 ); 
	} while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry));
	if(retry == 0) goto CLOSE;
	
/* ����Ϊ��FINWAIT-1 �ҷ�FIN + ACK */
/* ����Ϊ��FINWAIT-2 �Է�ACK */

	retry = TCP_RETRY_MAXNUM + 1;
	while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry));
	if(retry == 0) goto CLOSE;

/* ����Ϊ��FINWAIT-3 �Է�FIN + ACK */	
/* ����Ϊ��TIME-WAIT */

	info->squNumSnd = info->ackNumRcv;     /* ���͵�squӦ�õ��ڽ��յ�ack */
	info->ackNumSnd = info->squNumRcv + 1; /* �ر�֮ǰ�Է��϶���Ӧ��ACK(��������)������������ackӦ�õ����������͵����+0 */	
	TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
			 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
			 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
			 info->squNumSnd, /* ��ţ�ǰ���Ѿ�������� */
			 info->ackNumSnd, /* ȷ�Ϻţ�ǰ���Ѿ�������� */
			 (20 + 4) / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
			 TCP_FLAG_ACK | TCP_FLAG_FIN, /* ��Ӧ&���� */
			 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
			 0,               /* ����ָ�� */
			 optionMSS,       /* MSS */
			 optionMSS,       /* ����ָ�� */
			 0                /* ���ݳ��� */
			 ); 

	/* ���ӹرպ��ͷ���Դ */
CLOSE:	
	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_NOACK;  /* ʵ�������ǲ����ܷ����� */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return (retry) ? (mIP_OK) : (mIP_NOACK);
}

/**
 * @brief  �����ر�TCP���ӣ���Ӧ�Է���FIN+ACK
 * @param  info: �ص���������
 * @retval mIP_OK: �ɹ��ر� mIP_NOACK: �ͻ���û��Ӧ�����������ǻ�رյ����ӣ��ͷ���Դ 
 */
mIPErr TCP_coonCloseAck(TCPInfoStruct *info)
{
	UINT8 option[1];
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* ������Դ��� */
	UINT32 pos;
	UINT32 squ = info->ackNumRcv;
	
	do {
		TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
				 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 squ,               /* ��� */
				 info->squNumRcv + 1, /* ȷ�Ϻ� */
				 20 / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
				 TCP_FLAG_ACK,    /* ��Ӧ */
				 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
				 0,               /* ����ָ�� */
				 option,          /* MSS */
				 option,          /* ����ָ�� */
				 0                /* ���ݳ��� */
				 ); 

/* ����Ϊ�� CLOSE-WAIT */	
/* ����Ϊ�� LAST_ACK */
	
		TCP_send(info->srcIP,     /* Ŀ��IPΪԴIP */
				 info->dstPort,   /* Դ�˿�Ϊ�ͻ��˷��͵�Ŀ��˿� */
				 info->srcPort,   /* Ŀ��˿�Ϊ�ͻ��˵Ķ˿� */
				 squ,               /* ��� */
				 info->squNumRcv + 1, /* ȷ�Ϻ� */
				 20 / 4,    /* TCP�ײ����ȣ����е�20�ֽ�+MSS */
				 TCP_FLAG_ACK | TCP_FLAG_FIN, /* ��Ӧ&���� */
				 TCP_WINDOW_MAXSIZE, /* �����Ժ���Ҫ�޸� */
				 0,               /* ����ָ�� */
				 option,       /* MSS */
				 option,       /* ����ָ�� */
				 0                /* ���ݳ��� */
				 ); 
	} while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry) && (info->ackNumRcv == squ + 1));
	
	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_NOACK;  /* ʵ�������ǲ����ܷ����� */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return (retry) ? (mIP_OK) : (mIP_NOACK);
}

/**
 * @brief  �ͷ�����
 * @param  info: �ص���������
 * @retval mIP_OK: �ɹ��ͷ� mIP_ERROR: ���� 
 */
mIPErr TCP_coonRelease(TCPInfoStruct *info)
{
	UINT32 pos;

	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_ERROR;  /* ʵ�������ǲ����ܷ����� */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return mIP_OK;
}

/**
 * @brief  ����TCP����(ֻ�ܵ���һ��)
 * @param  ptr: TCP����ָ��
 * @param  cbPtr: �ص�����ָ�룬�������������Ժ�����ͻ��˷�������ϵͳ���Զ����ûص�����
 * @retval none
 */
void TCP_createTask(TCPTaskStruct *ptr, void (*cbPtr)(TCPInfoStruct *))
{
	UINT32 i;

	mIP.tcpTask = ptr;
	mIP.tcpCb = cbPtr;
	mIP.enFlag |= ENFLAG_TCP; /* ʹ��TCP */
	mIP.enFlag |= ENFLAG_UDP;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* ��ʼ���������ӵ�״̬Ϊ�ر� */
		mIP.tcpTask->state[i] = TCP_COON_CLOSED; 
	}
}

/**
 * @brief  �������е�TCP����״̬
 * @param  none
 * @retval none
 */
void TCP_coonReset(void)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		mIP.tcpTask->state[i] = TCP_COON_CLOSED;
	}
}

void tcpCoonOutput(void)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		printf("%d[%d] - ", i, mIP.tcpTask->state[i]);
	}
	printf("\r\n");
}



