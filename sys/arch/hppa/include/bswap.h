/*      $NetBSD: bswap.h,v 1.1.2.2 2002/06/23 17:37:07 jdolecek Exp $	*/

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
