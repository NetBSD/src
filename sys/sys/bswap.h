/*      $NetBSD: bswap.h,v 1.2.38.1 2005/02/12 18:17:55 yamt Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _MACHINE_BSWAP_H_	/* _BEFORE_ #ifndef _SYS_BSWAP_H_ */
#include <machine/bswap.h>
#endif

#ifndef _SYS_BSWAP_H_
#define _SYS_BSWAP_H_

#ifndef _LOCORE
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS
#if defined(_KERNEL) || defined(_STANDALONE) || !defined(__BSWAP_RENAME)
u_int16_t bswap16(u_int16_t);
u_int32_t bswap32(u_int32_t);
#else
u_int16_t bswap16(u_int16_t) __RENAME(__bswap16);
u_int32_t bswap32(u_int32_t) __RENAME(__bswap32);
#endif
u_int64_t bswap64(u_int64_t);
__END_DECLS
#endif /* !_LOCORE */

#endif /* !_SYS_BSWAP_H_ */
