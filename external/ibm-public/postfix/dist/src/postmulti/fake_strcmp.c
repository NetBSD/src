/*	$NetBSD: fake_strcmp.c,v 1.2.2.2 2026/05/11 17:13:56 martin Exp $	*/

/* System library. */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

/* Utility library. */
#include <msg.h>

/* Global library. */
#include <mail_params.h>

static int (*real_strcmp) (const char *, const char *);

/* find_real_func - libc lookup */

static void *find_real_func(const char *name)
{
    void   *sym;

    /*
     * XXX Casting a data pointer into a function pointer is non-portable.
     * Unfortunately, the dlfunc() function is available on FreeBSD but not
     * on Linux or Solaris. This is a cosmetic issue except on systems with
     * non-flat memory models.
     */
    if ((sym = dlsym(RTLD_NEXT, name)) == 0) {
	fprintf(stderr, "preload error for %s: %s\n", name, dlerror());
	exit(1);
    }
    return (sym);
}

int     strcmp(const char *s1, const char *s2)
{
    if (real_strcmp == 0)
	real_strcmp = find_real_func("strcmp");
    if (real_strcmp(DEF_CONFIG_DIR, s2) == 0)
	return (0);
    return real_strcmp(s1, s2);
}
