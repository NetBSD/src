#ifndef COMPAT_H_
#define COMPAT_H_

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_ASM_BYTEORDER_H
#include <asm/byteorder.h>
#endif

#ifdef HAVE_SYS_BYTEORDER_H
#  include <sys/byteorder.h>
#  if defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    undef _BIG_ENDIAN
#    define _BIG_ENDIAN	4321
#    define _BYTE_ORDER	_BIG_ENDIAN
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    undef _LITTLE_ENDIAN
#    define _LITTLE_ENDIAN	1234
#    define _BYTE_ORDER	_LITTLE_ENDIAN
#  endif
#endif

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
 
#ifdef HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#endif

#ifdef HAVE_LIBKERN_OSBYTEORDER_H
#include <libkern/OSByteOrder.h>
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#ifndef __UNCONST
#define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#endif

#ifdef HAVE_HTOBE64
#  define ISCSI_HTOBE64(x)	htobe64(x)
#  define ISCSI_BE64TOH(x)	be64toh(x)
#else
#  if defined(HAVE_LIBKERN_OSBYTEORDER_H)
#    define ISCSI_HTOBE64(x)	(x) = OSSwapBigToHostInt64((u_int64_t)(x))
#  elif _BYTE_ORDER == _BIG_ENDIAN
#    define ISCSI_HTOBE64(x)	(x)
#  elif defined(HAVE___BSWAP64)
#    define ISCSI_HTOBE64(x)	(x) = __bswap64((u_int64_t)(x))
#  else   /* LITTLE_ENDIAN */
#    define ISCSI_HTOBE64(x)	(((uint64_t)(ISCSI_NTOHL((uint32_t)(((x) << 32) >> 32))) << 32) | (uint32_t)ISCSI_NTOHL(((uint32_t)((x) >> 32))))
#  endif  /* LITTLE_ENDIAN */
#  define ISCSI_BE64TOH(x)	ISCSI_HTOBE64(x)
#endif

#ifndef _DIAGASSERT
#  ifndef __static_cast
#  define __static_cast(x,y) (x)y
#  endif
#define _DIAGASSERT(e) (__static_cast(void,0))
#endif

/* Added for busybox, which doesn't define INFTIM */
#ifndef INFTIM
#define INFTIM	-1
#endif

#endif /* COMPAT_H_ */
