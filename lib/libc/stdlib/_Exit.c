/*	$NetBSD: _Exit.c,v 1.1 2003/03/01 15:59:03 bjh21 Exp $	*/

/*
 * Ben Harris, 2002
 * This file is in the Public Domain
 */

#include <stdlib.h>
#include <unistd.h>

/*
 * IEEE 1003.1-2001 says:
 * The _Exit() and _exit() functions shall be functionally equivalent.
 */

void
_Exit(int status)
{

	_exit(status);
}
