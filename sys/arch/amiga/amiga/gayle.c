/*	$NetBSD: gayle.c,v 1.3 2001/01/19 21:18:29 is Exp $	*/

/* public domain */

/*
 * Gayle management routines
 *
 *   Any module that uses gayle should call gayle_init() before using anything
 *   related to gayle. gayle_init() can be called multiple times.
 */

#include <amiga/amiga/gayle.h>
#include <amiga/dev/zbusvar.h>

struct gayle_struct *gayle_base_virtual_address = 0;

#define GAYLE_PHYS_ADDRESS      0xda8000

void
gayle_init(void) {

	if (gayle_base_virtual_address != 0)
		return;

	gayle_base_virtual_address =
		(struct gayle_struct *) ztwomap(GAYLE_PHYS_ADDRESS);
}
