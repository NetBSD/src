/* utility definitions */
#define	DUPMAX		_POSIX2_RE_DUP_MAX	/* xxx is this right? */
#define	INFINITY	(DUPMAX + 1)
#define	NC		(CHAR_MAX - CHAR_MIN + 1)

typedef unsigned char uch;

#ifndef REDEBUG
#ifndef NDEBUG
#define	NDEBUG	/* no assertions please */
#endif
#endif
#include <assert.h>
