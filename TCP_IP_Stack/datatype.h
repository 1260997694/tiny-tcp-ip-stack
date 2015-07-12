#ifndef __DATATYPE_H
#define __DATATYPE_H

typedef char            CHAR;

typedef unsigned char  UINT8;
typedef char            INT8;

typedef unsigned short UINT16;
typedef          short  INT16;

typedef unsigned long  UINT32;
typedef          long   INT32;

typedef enum mIPErr{
	mIP_OK = 0, /* �ɹ� */
	mIP_ERROR,  /* ʧ�� */
	mIP_NOACK,  /* ��Ӧ�� */
	mIP_RST,    /* ��λ */
	mIP_FIN     /* �ر� */
} mIPErr;

typedef enum VND_METHOD {
	METHOD_FATFS, /* fatfs�ļ�ϵͳ */
	METHOD_RAM    /* RAM */
} VND_METHOD;

#endif
