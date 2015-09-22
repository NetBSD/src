/*	$NetBSD: wdogvar.h,v 1.6.182.1 2015/09/22 12:05:51 skrll Exp $	*/

#ifndef _SH3_WDOGVAR_H_
#define	_SH3_WDOGVAR_H_

#include <sys/ioccom.h>

#define	WDOGF_OPEN	1

#define	SIORESETWDOG	_IO('S', 0x0)
#define	SIOSTARTWDOG	_IO('S', 0x1)
#define	SIOSTOPWDOG	_IO('S', 0x2)
#define	SIOSETWDOG	_IOW('S', 0x3, int)
#define	SIOWDOGSETMODE	_IOW('S', 0x4, int)

#define	WDOGM_RESET	1
#define	WDOGM_INTR	2

#ifdef _KERNEL
extern unsigned int maxwdog;
extern void wdog_wr_cnt(unsigned char);
extern void wdog_wr_csr(unsigned char);
#endif

#endif /* !_SH3_WDOGVAR_H_ */
