/*	$NetBSD: mbr_macppc.S,v 1.1 2002/06/17 15:25:45 itojun Exp $	*/

/*
 * macppc master boot code - generates /usr/mdec/mbr identical to OpenBSD's.
 * not sure what it means, but it makes OpenFirmware happy
 */

#include <machine/asm.h>

	.text
ENTRY(start)
	. = _C_LABEL(start) + 0x1be
	.byte	0x80
	. = _C_LABEL(start) + 0x1c2
	.byte	0x06
	. = _C_LABEL(start) + 0x1c6
	.byte	0x01
	. = _C_LABEL(start) + 0x1cb
	.byte	0x08
	. = _C_LABEL(start) + 0x1ef
	.byte	0x06
	. = _C_LABEL(start) + 0x1f0
	.byte	0x22, 0x02, 0xa6, 0x06, 0xb6, 0xa7, 0x01, 0x08
	.byte	0x00, 0x00, 0x59, 0x6f, 0x1f, 0x00, 0x55, 0xaa
