/*	$NetBSD: sig32_machdep.c,v 1.1.2.2 2015/04/06 15:18:01 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sig32_machdep.c,v 1.1.2.2 2015/04/06 15:18:01 skrll Exp $");

#define	COMPATNAME1(x)		__CONCAT(netbsd32_,x)
#define	COMPATNAME2(x)		__CONCAT(x,32)
#define	COMPATTYPE(x)		__CONCAT(x,32_t)
#define	COMPATINC		<compat/netbsd32/netbsd32.h>
#define	UCLINK_SET(uc,c)	((uc)->uc_link = (uint32_t)(intptr_t)(c))
#define	sendsig_siginfo		netbsd32_sendsig

#define	COPY_SIGINFO(d,s)	do { \
		siginfo_t si = { ._info = (s)->ksi_info, }; \
		netbsd32_si_to_si32(&(d)->sf_si, &si); \
	} while (/*CONSTCOND*/ 0)

#include "sig_machdep.c"
