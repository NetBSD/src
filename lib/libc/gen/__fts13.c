/*	$NetBSD: __fts13.c,v 1.45 2005/08/19 02:04:54 christos Exp $	*/

#include "namespace.h"
#include <sys/cdefs.h>
#include <dirent.h>

#define __LIBC12_SOURCE__

__warn_references(__fts_children13,
    "warning: reference to compatibility __fts_children13();"
    " include <fts.h> for correct reference")
__warn_references(__fts_close13,
    "warning: reference to compatibility __fts_close13();"
    " include <fts.h> for correct reference")
__warn_references(__fts_open13,
    "warning: reference to compatibility __fts_open13();"
    " include <fts.h> for correct reference")
__warn_references(__fts_read13,
    "warning: reference to compatibility __fts_read13();"
    " include <fts.h> for correct reference")
__warn_references(__fts_set13,
    "warning: reference to compatibility __fts_set13();"
    " include <fts.h> for correct reference")

#include <sys/stat.h>

#define stat		__stat13
#define lstat		__lstat13
#define fstat		__fstat13

#define	__fts_stat_t	struct stat13
#define	__fts_nlink_t	nlink_t
#define	__fts_ino_t	u_int32_t

#undef fts_children
#define fts_children __fts_children13
#undef fts_close
#define fts_close __fts_close13
#undef fts_open
#define fts_open  __fts_open13
#undef fts_read
#define fts_read __fts_read13
#undef fts_set
#define fts_set __fts_set13

#include "__fts30.c"
