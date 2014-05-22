#include "config.h"

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "Id: iswblank.c,v 1.1 2001/10/11 19:22:29 skimo Exp ";
#endif /* LIBC_SCCS and not lint */
#else
__RCSID("$NetBSD: iswblank.c,v 1.2.4.2 2014/05/22 15:50:33 yamt Exp $");
#endif

#include <wchar.h>
#include <wctype.h>

int
iswblank (wint_t wc)
{
    return iswctype(wc, wctype("blank"));
}
