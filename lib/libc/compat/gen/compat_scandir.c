/*	$NetBSD: compat_scandir.c,v 1.2 2016/12/16 04:45:04 mrg Exp $	*/

#include <sys/stat.h>
#define __LIBC12_SOURCE__
#include "namespace.h"
#include <dirent.h>
#include <compat/include/dirent.h>

#ifdef __weak_alias
__weak_alias(scandir,_scandir)
#endif

#ifdef __warn_references
__warn_references(scandir,
    "warning: reference to compatibility scandir(); include <dirent.h> for correct reference")
#endif

#define dirent dirent12
#define COMPARARG void *

#include "gen/scandir.c"
