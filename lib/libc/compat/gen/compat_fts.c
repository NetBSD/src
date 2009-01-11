/*	$NetBSD: compat_fts.c,v 1.3 2009/01/11 02:46:25 christos Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, October 21, 1997.
 * Public domain.
 */

#include "namespace.h"
#include <sys/cdefs.h>
#include <dirent.h>

#define __LIBC12_SOURCE__
#include <sys/stat.h>
#include <compat/sys/time.h>
#include <compat/sys/stat.h>

#define __fts_stat_t	struct stat12
#define __fts_nlink_t	u_int16_t
#define __fts_ino_t	u_int32_t

#include <fts.h>
#include <compat/include/fts.h>

#ifdef __weak_alias
__weak_alias(fts_children,_fts_children)
__weak_alias(fts_close,_fts_close)
__weak_alias(fts_open,_fts_open)
__weak_alias(fts_read,_fts_read)
__weak_alias(fts_set,_fts_set)
#endif /* __weak_alias */

__warn_references(fts_children,
    "warning: reference to compatibility fts_children();"
    " include <fts.h> for correct reference")
__warn_references(fts_close,
    "warning: reference to compatibility fts_close();"
    " include <fts.h> for correct reference")
__warn_references(fts_open,
    "warning: reference to compatibility fts_open();"
    " include <fts.h> for correct reference")
__warn_references(fts_read,
    "warning: reference to compatibility fts_read();"
    " include <fts.h> for correct reference")
__warn_references(fts_set,
    "warning: reference to compatibility fts_set();"
    " include <fts.h> for correct reference")

#define	__FTS_COMPAT_TAILINGSLASH

#include "gen/fts.c"
