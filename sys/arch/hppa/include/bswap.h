/*      $NetBSD: bswap.h,v 1.1 2002/06/05 01:04:21 fredette Exp $	*/

/* Written by Manuel Bouyer. Public domain */

#ifndef _HPPA_BSWAP_H_
#define	_HPPA_BSWAP_H_

#define __BSWAP_RENAME
#include <sys/bswap.h>

#ifdef  __GNUC__

#include <machine/byte_swap.h>
#define bswap16(x)      __byte_swap_word(x)
#define bswap32(x)      __byte_swap_long(x)

#endif /* __GNUC__ */

#endif /* !_HPPA_BSWAP_H_ */
