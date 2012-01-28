
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */


#ifndef _TDDL_H_
#define _TDDL_H_

#include <threads.h>
#include "tcsd_wrap.h"
#include "tcsd.h"

struct tpm_device_node {
	char *path;
#define TDDL_TRANSMIT_IOCTL	1
#define TDDL_TRANSMIT_RW	2
	int transmit;
	int fd;
};

#define TDDL_TXBUF_SIZE		2048
#define TDDL_UNDEF		-1

TSS_RESULT Tddli_Open(void);

TSS_RESULT Tddli_TransmitData(BYTE *pTransmitBuf,
			UINT32 TransmitBufLen,
			BYTE *pReceiveBuf,
			UINT32 *pReceiveBufLen);

TSS_RESULT Tddli_Close(void);

#endif
