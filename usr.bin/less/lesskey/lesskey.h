/*	$NetBSD: lesskey.h,v 1.1.1.4 1999/04/06 05:30:39 mrg Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996,1999  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Format of a lesskey file:
 *
 *	LESSKEY_MAGIC (4 bytes)
 *	 sections...
 *	END_LESSKEY_MAGIC (4 bytes)
 *
 * Each section is:
 *
 *	section_MAGIC (1 byte)
 *	section_length (2 bytes)
 *	key table (section_length bytes)
 */
#define	C0_LESSKEY_MAGIC	'\0'
#define	C1_LESSKEY_MAGIC	'M'
#define	C2_LESSKEY_MAGIC	'+'
#define	C3_LESSKEY_MAGIC	'G'

#define	CMD_SECTION		'c'
#define	EDIT_SECTION		'e'
#define	VAR_SECTION		'v'
#define	END_SECTION		'x'

#define	C0_END_LESSKEY_MAGIC	'E'
#define	C1_END_LESSKEY_MAGIC	'n'
#define	C2_END_LESSKEY_MAGIC	'd'

/* */
#define	KRADIX		64
