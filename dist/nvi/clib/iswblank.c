#include "config.h"

#if defined(LIBC_SCCS) && !defined(lint)
static const char sccsid[] = "$Id: iswblank.c,v 1.1.1.1.2.2 2008/05/18 12:29:22 yamt Exp $";
#endif /* LIBC_SCCS and not lint */

#include <wchar.h>
#include <wctype.h>

int
iswblank (wint_t wc)
{
    return iswctype(wc, wctype("blank"));
}
