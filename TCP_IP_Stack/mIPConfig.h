#ifndef __MIPCONFIG_H
#define __MIPCONFIG_H

#define MYTCPIP_USE_FTP    1


/* ��������С */
#define myICPIP_bufSize    1550 /* ���1550�������� */
#define myTCPIP_datBufSize 1550

/* ARP������Ŀ������� */
#define ARP_CACHE_MAXNUM     5

/* ARP�������ʱ��(��λΪ��) */
#define ARP_CACHE_UPDATETIME 300

/* �ȴ�ARP��ѯ��Ӧ��ʱ��(��λΪ����),������ʱ����Ϊ��������û����Ӧ���� */
#define ARP_TIMEWAIT         500

/* TCP���ڵ����ֵ */
#define TCP_WINDOW_MAXSIZE   1460

/* TCP����ĳ��� */
#define TCP_OPTION_MSS       1460

/* ����ܽ�����TCP�������� */
#define TCP_COON_MAXNUM      2 /* �����Ե��߳�ʱHTTP���Ϊ1��FTPΪ2 */

/* TCP�ȴ���Ӧ�ĳ�ʱʱ��(��λΪ����) */
#define TCP_RETRY_TIME       500

/* TCP��ʱ���������� */
#define TCP_RETRY_MAXNUM     5

#endif
