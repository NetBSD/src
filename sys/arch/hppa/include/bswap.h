/*      $NetBSD: bswap.h,v 1.1.4.2 2002/07/14 17:47:18 gehenna Exp $	*/

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
