/*      $NetBSD: bswap.h,v 1.2.76.1 2009/05/04 08:10:33 yamt Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _X86_64_BSWAP_H_
#define	_X86_64_BSWAP_H_

#ifdef __x86_64__

#include <machine/byte_swap.h>

#define __BSWAP_RENAME
#include <sys/bswap.h>

#else	/*	__x86_64__	*/

#include <i386/bswap.h>

#endif	/*	__x86_64__	*/

#endif /* !_X86_64_BSWAP_H_ */
