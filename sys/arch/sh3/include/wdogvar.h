/* $NetBSD: wdogvar.h,v 1.3 2000/10/31 02:04:13 msaitoh Exp $ */

#ifndef _SH3_WDOGVAR_H_
#define _SH3_WDOGVAR_H_

#define WDOGF_OPEN	1

#define SIORESETWDOG	_IO('S', 0x0)
#define SIOSTARTWDOG	_IO('S', 0x1)
#define SIOSTOPWDOG	_IO('S', 0x2)
#define	SIOSETWDOG	_IOW('S', 0x3, int)
#define SIOWDOGSETMODE	_IOW('S', 0x4, int)

#define WDOGM_RESET	1
#define WDOGM_INTR	2

#ifdef _KERNEL
extern unsigned int maxwdog;
extern void wdog_wr_cnt __P((unsigned char));
extern void wdog_wr_csr __P((unsigned char));
#endif

#endif /* !_SH3_WDOGVAR_H_ */
