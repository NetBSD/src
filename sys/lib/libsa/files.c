/* $NetBSD: files.c,v 1.1.10.2 2002/06/23 17:49:52 jdolecek Exp $ */

/*
 *	files.c:
 *
 *	libsa file table.  separate from other global variables so that
 *	all of those don't need to be linked in just to use open, et al.
 */

#include "stand.h"

struct open_file files[SOPEN_MAX];
