/*	$NetBSD: macglobals.s,v 1.1 1994/12/03 23:34:53 briggs Exp $	*/

/* Copyright 1994 by Bradley A. Grantham, All rights reserved */

/*
 * MacOS global variable space; storage for global variables used by
 * MacROMGlue routines (see macrom.c, macrom.h macromasm.s)
 */

	.text
	.space 0xF00	/* did I miss something? this is a bad fix for
	.space 0x1000	/* did I miss something? this is a bad fix for
			   someone who is writing over low mem */
