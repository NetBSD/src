/* $NetBSD: wcstoimax.c,v 1.1 2003/03/11 09:21:23 tshiozak Exp $ */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: wcstoimax.c,v 1.1 2003/03/11 09:21:23 tshiozak Exp $");
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

#define	_FUNCNAME	wcstoimax
#define	__INT		intmax_t
#define	__INT_MIN	INTMAX_MIN
#define	__INT_MAX	INTMAX_MAX

#include "_wcstol.h"
