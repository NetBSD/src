/* $NetBSD: extern.h,v 1.1.2.1 2024/06/29 19:43:25 perseant Exp $	 */

#include <stdarg.h>
#include "vnode.h"

void pwarn(const char *, ...);
int reply(const char *);

extern int nvnodes;
extern void (*panic_func)(int, const char *, va_list);
