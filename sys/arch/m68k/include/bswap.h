/*      $NetBSD: bswap.h,v 1.1 1999/01/15 13:31:24 bouyer Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _M68K_BSWAP_H_
#define _M68K_BSWAP_H_

#include <sys/cdefs.h>

__BEGIN_DECLS
u_int16_t       bswap16 __P((u_int16_t));
u_int32_t       bswap32 __P((u_int32_t));
u_int64_t       bswap64 __P((u_int64_t));
__END_DECLS

#endif /* _M68K_BSWAP_H_ */
