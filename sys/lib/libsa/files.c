/* $NetBSD: files.c,v 1.2 2022/04/24 06:52:59 mlelstv Exp $ */

/*
 *	files.c:
 *
 *	libsa file table.  separate from other global variables so that
 *	all of those don't need to be linked in just to use open, et al.
 */

#include "stand.h"

struct open_file files[SOPEN_MAX];
const char *fsmod = NULL;	/* file system module name to load */
