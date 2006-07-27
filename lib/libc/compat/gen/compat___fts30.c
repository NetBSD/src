/*	$NetBSD: compat___fts30.c,v 1.2 2006/07/27 15:46:30 christos Exp $	*/

#include "namespace.h"
#include <sys/cdefs.h>
#include <dirent.h>

__warn_references(__fts_children30,
    "warning: reference to compatibility __fts_children30();"
    " include <fts.h> for correct reference")
__warn_references(__fts_close30,
    "warning: reference to compatibility __fts_close30();"
    " include <fts.h> for correct reference")
__warn_references(__fts_open30,
    "warning: reference to compatibility __fts_open30();"
    " include <fts.h> for correct reference")
__warn_references(__fts_read30,
    "warning: reference to compatibility __fts_read30();"
    " include <fts.h> for correct reference")
__warn_references(__fts_set30,
    "warning: reference to compatibility __fts_set30();"
    " include <fts.h> for correct reference")

#include <sys/stat.h>

#define	__fts_length_t	u_short
#define	__fts_number_t	long

#undef fts_children
#define fts_children __fts_children30
#undef fts_close
#define fts_close __fts_close30
#undef fts_open
#define fts_open  __fts_open30
#undef fts_read
#define fts_read __fts_read30
#undef fts_set
#define fts_set __fts_set30

#define __LIBC12_SOURCE__
#include <fts.h>
#include <compat/include/fts.h>

#define	__FTS_COMPAT_LENGTH

#include "gen/fts.c"
