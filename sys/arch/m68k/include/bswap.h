/*      $NetBSD: bswap.h,v 1.3 2001/03/30 20:00:05 leo Exp $      */

#ifndef _MACHINE_BSWAP_H_
#define	_MACHINE_BSWAP_H_

#define	__BSWAP_RENAME
#include <sys/bswap.h>

#ifdef	__GNUC__

#include <m68k/byte_swap.h>

#define bswap16(x)	__byte_swap_word(x)
#define bswap32(x)	__byte_swap_long(x)

#endif	/* __GNUC__ */

#endif /* !_MACHINE_BSWAP_H_ */
