/*	$NetBSD: label.S,v 1.1 2003/04/16 22:17:44 dsl Exp $	*/

#include <machine/asm.h>

/*
 * This fills in the space taken by the NetBSD disklabel in the first
 * NetBSD partition on the disk.
 * However it is possible that we are booting from a subsequent
 * NetBSD partition, so must not access the disklabe in this space.
 */

	.text
label:	.fill	512
