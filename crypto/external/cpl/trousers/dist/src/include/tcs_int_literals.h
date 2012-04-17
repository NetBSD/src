
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
 *
 */

#ifndef _TCS_INT_LITERALS_H_
#define _TCS_INT_LITERALS_H_

#define	TPM_VENDOR_UNKNOWN  0
#define TPM_VENDOR_ATMEL	1
#define TPM_VENDOR_IFX		2
#define TPM_VENDOR_NATL		3

#define TPM_PARAMSIZE_OFFSET 0x02

#define NULL_TPM_HANDLE	((TCPA_KEY_HANDLE)-1)
#define NULL_TCS_HANDLE	((TCS_KEY_HANDLE)-1)
#define SRK_TPM_HANDLE	(0x40000000)
#define EK_TPM_HANDLE	(0x40000001)

#define FIXED_TCS_MANUFACTURER "IBM "

#endif
