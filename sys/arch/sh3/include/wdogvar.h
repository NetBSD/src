/* $NetBSD: wdogvar.h,v 1.3.2.2 2000/11/20 20:24:31 bouyer Exp $ */

#ifndef _SH3_WDOGVAR_H_
#define _SH3_WDOGVAR_H_

#define WDOGF_OPEN	1

#define SIORESETWDOG	_IO('S', 0x0)
#define SIOSTARTWDOG	_IO('S', 0x1)
#define SIOSTOPWDOG	_IO('S', 0x2)
#define	SIOSETWDOG	_IOW('S', 0x3, int)

#ifdef _KERNEL
extern void wdog_wr_cnt __P((unsigned char));
extern void wdog_wr_csr __P((unsigned char));
#endif

#endif /* !_SH3_WDOGVAR_H_ */
