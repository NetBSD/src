/*      $NetBSD: bswap.h,v 1.5 2005/12/26 18:41:36 perry Exp $      */

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
uint16_t bswap16(uint16_t);
uint32_t bswap32(uint32_t);
#else
uint16_t bswap16(uint16_t) __RENAME(__bswap16);
uint32_t bswap32(uint32_t) __RENAME(__bswap32);
#endif
uint64_t bswap64(uint64_t);
__END_DECLS
#endif /* !_LOCORE */

#endif /* !_SYS_BSWAP_H_ */
