#include "config.h"

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "$Id: iswblank.c,v 1.1.1.1 2008/05/16 18:03:11 aymeric Exp $";
#endif /* LIBC_SCCS and not lint */

#include <wchar.h>
#include <wctype.h>

int
iswblank (wint_t wc)
{
    return iswctype(wc, wctype("blank"));
}
