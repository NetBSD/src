/* $NetBSD: files.c,v 1.1.4.2 2002/04/01 07:48:08 nathanw Exp $ */

/*
 *	files.c:
 *
 *	libsa file table.  separate from other global variables so that
 *	all of those don't need to be linked in just to use open, et al.
 */

#include "stand.h"

struct open_file files[SOPEN_MAX];
