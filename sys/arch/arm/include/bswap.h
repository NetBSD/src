/*      $NetBSD: bswap.h,v 1.4.122.1 2014/08/20 00:02:46 tls Exp $      */

#ifndef _ARM_BSWAP_H_
#define	_ARM_BSWAP_H_

#ifdef __aarch64__
#include <aarch64/byte_swap.h>
#else
#include <arm/byte_swap.h>
#endif

#define __BSWAP_RENAME
#include <sys/bswap.h>

#endif /* !_ARM_BSWAP_H_ */
