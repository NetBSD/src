/*      $NetBSD: bswap.h,v 1.1.6.1 2002/09/06 08:32:34 jdolecek Exp $      */

#ifndef _MACHINE_BSWAP_H_
#define	_MACHINE_BSWAP_H_

#define __BSWAP_RENAME
#include <sys/bswap.h>

#ifdef __GNUC__

#include <arm/byte_swap.h>
#define	bswap16(x)	__byte_swap_16(x)
#define	bswap32(x)	__byte_swap_32(x)

#endif /* __GNUC__ */

#endif /* !_MACHINE_BSWAP_H_ */
