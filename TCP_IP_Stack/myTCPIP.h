#ifndef __MYTCPIP_H
#define __MYTCPIP_H

#include "datatype.h"
#include "mIPConfig.h"
#include "driver.h"
#include "share.h"
#include "ETH.h"
#include "ARP.h"
#include "IP.h"
#include "ICMP.h"
#include "UDP.h"
#include "TCP.h"
#include "TCP_virtualWindow.h"



/* TCP */
#define TCP_FLAG_URG 0x20 /* ����URG����ʾ���ݾ����൱�ߵ����ȼ������ȷ���,��Ҫ�ͽ���ָ�����ʹ�� */
#define TCP_FLAG_ACK 0x10 /* ȷ��ACK���������Ӻ����б��Ķ�ACK������1 */
#define TCP_FLAG_PSH 0x08 /* ����PSH������ʹ�� */
#define TCP_FLAG_RST 0x04 /* ��λRST����λ��ʾTCP�����г������ش��󣬱����ͷ����ӣ������½��� */
#define TCP_FLAG_SYN 0x02 /* ͬ��SYN����SYN = 1, ACK = 0,���ʾ���������ӣ��Է���Ӧ��ʱ��SYN = 1, ACK = 1 */
#define TCP_FLAG_FIN 0x01 /* ��ֹFIN���ͷ����� */

#define TCP_COON_CLOSED    0 /* TCP-�ѹر� */
#define TCP_COON_SYNRCV    1 /* TCP-ͬ���յ� */
#define TCP_COON_ESTAB     2 /* TCP-�ѽ������� */
#define TCP_COON_CLOSEWAIT 3 /* TCP-�رյȴ� */
#define TCP_COON_LASTACK   4 /* TCP-�رյ����ȷ�� */

#define TCP_PORT_HTTP   80 
#define TCP_PORT_FTP    21
#define TCP_PORT_TFTP   69
#define TCP_PORT_TELNET 23
 
/* IP */
#define IP_VER_IPV4      4
#define IP_VER_IPV6      6
#define IP_HEADLENGTH    5      /* 5*4 = 20 */
#define IP_FLAG_MF       0x01   /* ���з�Ƭ */
#define IP_FLAG_DF       0x02   /* ���ܷ�Ƭ */
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_IGMP 2
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_EGP  8
#define IP_PROTOCOL_IGP  9
#define IP_PROTOCOL_UDP  17
#define IP_PROTOCOL_IPV6 41
#define IP_PROTOCOL_OSPF 89

/* ICMP */
#define ICMP_TYPE_REPLY 0  /* ICMPӦ�� */
#define ICMP_CODE_REPLY 0  /* ICMPӦ����� */
#define ICMP_TYPE_REQ   8  /* ICMP Echo����(ping) */
#define ICMP_CODE_REQ   0  /* ICMP Echo�������(ping) */

/* ARP */
#define ARP_OPCODE_REQ   0x0001  /* ���� */
#define ARP_OPCODE_REPLY 0x0002  /* ��Ӧ */

/* ETH */
#define HARDWARE_TYPE_ETH  0x0001

/* protocol type */
#define PROTOCOL_TYPE_ARP  0x0806
#define PROTOCOL_TYPE_IP   0x0800
#define PROTOCOL_TYPE_IPV6 0x86DD

/* enFlag */
#define ENFLAG_TCP 0x01 /* ʹ��TCP */
#define ENFLAG_UDP 0x02 /* ʹ��UDP */

/* ARP����ṹ�� */
typedef struct ARPCacheStruct {
	UINT16 num;                       /* Ŀǰ�������洢��Ŀ����������ʼΪ0 */
	UINT8  mac[ARP_CACHE_MAXNUM][6];  /* �����MAC��ַ */
	UINT8  ip[ARP_CACHE_MAXNUM][4];   /* �����IP��ַ��IP�е�Ԫ��MAC�е�Ԫ��һһ��Ӧ�� */	
	UINT32 arpUpdataTime;             /* ARP����ʱ�� */
} ARPCacheStruct;

/* Э��ջ�ṹ�� */
typedef struct myTCPIPStruct {
	UINT8  *buf;             /* ��BUF���洢��������յ����ݰ� */
	UINT8  *datBuf;          /* ����buf���洢��ʱ���� */
	UINT8  mac[6];           /* ����MAC��ַ */
	UINT8  macTmp[6];        /* �洢�ϴν��յ��Ŀͻ���MAC */
	UINT8  ip[4];            /* ����IP��ַ */
	UINT8  ipTmp[4];         /* �洢�ϴν��յ��Ŀͻ���IP */
	UINT16 identification;   /* ϵͳά���ı�־�����ݲ����Ƭʱ����һ��IP��1 */
	UINT8  enFlag;           /* ʹ�ܱ�־�������Ƿ�ʹ�õ�TCP����UDP */
	
	ARPCacheStruct arpCache; /* ARP���ٻ��� */
	TCPTaskStruct *tcpTask;   /* TCP����ָ�� */
	void (*tcpCb)(TCPInfoStruct *); /* TCP�ص�����ָ�� */
} myTCPIPStruct;


mIPErr myTCPIP_init(UINT8 *mac, UINT8 *ip, UINT8 *buf, UINT8 *datBuf);
void   myTCPIP_process(void);

#endif

