#ifndef COMPAT_H_
#define COMPAT_H_

#include "config.h"

#include <sys/types.h>

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif

#ifndef __UNCONST
#define __UNCONST(a)    ((void *)(unsigned long)(const void *)(a))
#endif

#ifndef HTOBE64
#  if _BYTE_ORDER == _BIG_ENDIAN
#    define HTOBE64(x)      (void) (x)
#  else   /* LITTLE_ENDIAN */
#    define HTOBE64(x)      (x) = __bswap64((u_int64_t)(x))
#    define bswap64(x)      __bswap64(x)
#  endif  /* LITTLE_ENDIAN */
#endif

#ifndef BE64TOH
#define BE64TOH(x)      HTOBE64(x)
#endif

#ifndef _DIAGASSERT
#  ifndef __static_cast
#  define __static_cast(x,y) (x)y
#  endif
#define _DIAGASSERT(e) (__static_cast(void,0))
#endif

#endif /* COMPAT_H_ */
