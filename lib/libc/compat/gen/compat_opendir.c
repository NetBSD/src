/*	$NetBSD: compat_opendir.c,v 1.2 2019/09/22 22:59:38 christos Exp $	*/

#include "namespace.h"
#include <sys/stat.h>
#include <signal.h>

#define __LIBC12_SOURCE__
#include <dirent.h>
#include <compat/include/dirent.h>
#include <compat/sys/statvfs.h>

#ifdef __weak_alias
__weak_alias(opendir,_opendir)
#endif

#ifdef __warn_references
__warn_references(opendir,
    "warning: reference to compatibility opendir(); include <dirent.h> for correct reference")
__warn_references(__opendir2,
    "warning: reference to compatibility __opendir2(); include <dirent.h> for correct reference")
#endif

#define dirent dirent12
#define	fstatvfs1 __fstatvfs190

#include "gen/opendir.c"
