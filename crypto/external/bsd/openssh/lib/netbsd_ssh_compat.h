/*	$NetBSD: netbsd_ssh_compat.h,v 1.1.6.2 2017/08/15 05:27:53 snj Exp $	*/

/*
 * compat header for netbsd-6 and modern ssh, provides versions of
 * modern netbsd libc functionality.
 */

#include <sys/types.h>
#include <inttypes.h>

int consttime_memequal(const void *, const void *, size_t);
void *explicit_memset(void *, int, size_t);
int reallocarr(void *, size_t, size_t);
void *reallocarray(void *, size_t, size_t);
intmax_t strtoi(const char * restrict nptr, char ** restrict endptr,
		int base, intmax_t lo, intmax_t hi, int *rstatus);
long long strtonum(const char *, long long, long long, const char **);

#define RB_FOREACH_SAFE(x, name, head, y)                               \
	for ((x) = RB_MIN(name, head);                                  \
	    ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL);    \
	     (x) = (y))
