/*	$NetBSD: endian.h,v 1.1 1999/03/31 00:44:48 fvdl Exp $	*/

#include <machine/endian.h>
#include <machine/bswap.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#define le_to_native16(x)	(x)
#define le_to_native32(x)	(x)
#define le_to_native64(x)	(x)

#define native_to_le16(x)	(x)
#define native_to_le32(x)	(x)
#define native_to_le64(x)	(x)

#else

#define le_to_native16(x)	bswap16(x)
#define le_to_native32(x)	bswap32(x)
#define le_to_native64(x)	bswap64(x)

#define native_to_le16(x)	bswap16(x)
#define native_to_le32(x)	bswap32(x)
#define native_to_le64(x)	bswap64(x)

#endif
