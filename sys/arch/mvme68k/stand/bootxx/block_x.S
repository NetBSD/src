/*	$NetBSD: block_x.S,v 1.1 2000/12/04 18:44:51 scw Exp $	*/

#include <machine/asm.h>
#include "bootxx.h"

	.text

GLOBAL(block_size)
	.long	512

GLOBAL(block_count)
	.long	MAXBLOCKNUM

GLOBAL(block_table)
	.space	(MAXBLOCKNUM * 4)
