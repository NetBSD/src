/*	$NetBSD: wcstoumax.c,v 1.1 2003/03/11 09:21:24 tshiozak Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: wcstoumax.c,v 1.1 2003/03/11 09:21:24 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "__wctoint.h"

#define	_FUNCNAME	wcstoumax
#define	__UINT		uintmax_t
#define	__UINT_MAX	UINTMAX_MAX

#include "_wcstoul.h"
