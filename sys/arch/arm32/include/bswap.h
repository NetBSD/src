/*      $NetBSD: bswap.h,v 1.1 1999/01/15 13:31:23 bouyer Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _MACHINE_BSWAP_H_
#define _MACHINE_BSWAP_H_

#include <sys/cdefs.h>

__BEGIN_DECLS
#ifdef _KERNEL
u_int16_t       bswap16 __P((u_int16_t));
u_int32_t       bswap32 __P((u_int32_t));
#else
u_int16_t       bswap16 __P((u_int16_t)) __RENAME(__bswap16);
u_int32_t       bswap32 __P((u_int32_t)) __RENAME(__bswap32);
#endif
u_int64_t       bswap64 __P((u_int64_t));
__END_DECLS

#endif /* _MACHINE_BSWAP_H_ */
