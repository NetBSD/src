/*	$NetBSD: compat___fts32.c,v 1.1.2.1 2008/11/09 19:36:03 christos Exp $	*/

#include "namespace.h"
#include <sys/cdefs.h>
#include <dirent.h>

#define __LIBC12_SOURCE__
__warn_references(__fts_children32,
    "warning: reference to compatibility __fts_children32();"
    " include <fts.h> for correct reference")
__warn_references(__fts_close32,
    "warning: reference to compatibility __fts_close32();"
    " include <fts.h> for correct reference")
__warn_references(__fts_open32,
    "warning: reference to compatibility __fts_open32();"
    " include <fts.h> for correct reference")
__warn_references(__fts_read32,
    "warning: reference to compatibility __fts_read32();"
    " include <fts.h> for correct reference")
__warn_references(__fts_set32,
    "warning: reference to compatibility __fts_set32();"
    " include <fts.h> for correct reference")

#define	__fts_stat_t	struct stat30
#define	__fts_dev_t	uint32_t
#undef fts_children
#define fts_children __fts_children32
#undef fts_close
#define fts_close __fts_close32
#undef fts_open
#define fts_open  __fts_open32
#undef fts_read
#define fts_read __fts_read32
#undef fts_set
#define fts_set __fts_set32

#include <sys/time.h>
#include <compat/sys/time.h>
#include <sys/stat.h>
#include <compat/sys/stat.h>

#include <fts.h>
#include <compat/include/fts.h>

#define	__FTS_COMPAT_LENGTH

#define stat		__stat30
#define lstat		__lstat30
#define fstat		__fstat30

#include "gen/fts.c"
