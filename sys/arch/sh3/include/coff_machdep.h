/*	$NetBSD: coff_machdep.h,v 1.3 2000/06/04 16:24:01 mycroft Exp $	*/

/*
 * Copyright (c) 1994, 1995 Scott Bartram
 * All rights reserved.
 *
 * adapted from sys/sys/exec_ecoff.h
 * based on Intel iBCS2
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_COFF_MACHDEP_H_
#define _SH3_COFF_MACHDEP_H_

/* f_magic flags */
#define COFF_MAGIC_SH3_BIG	0x500
#define COFF_MAGIC_SH3_LITTLE	0x550

/* magic */
#define COFF_OMAGIC	0444	/* text not write-protected; data seg
				   is contiguous with text */
#define COFF_NMAGIC	0410	/* text is write-protected; data starts
				   at next seg following text */
#define COFF_ZMAGIC	0000	/* text and data segs are aligned for
				   direct paging */
#define COFF_SMAGIC	0443	/* shared lib */

#define COFF_LDPGSZ 4096

#define COFF_SEGMENT_ALIGNMENT(fp, ap) \
    (((fp)->f_flags & COFF_F_EXEC) == 0 ? 4 : 16)

#ifndef BYTE_ORDER
#error Define BYTE_ORDER!
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define COFF_BADMAG(ex) ((ex)->f_magic != COFF_MAGIC_SH3_BIG)
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
#define COFF_BADMAG(ex) ((ex)->f_magic != COFF_MAGIC_SH3_LITTLE)
#endif

#define IBCS2_HIGH_SYSCALL(n)		(((n) & 0x7f) == 0x28)
#define IBCS2_CVT_HIGH_SYSCALL(n)	(((n) >> 8) + 128)

#ifdef DEBUG_COFF
#define DPRINTF(a)      printf a;
#else
#define DPRINTF(a)
#endif

#define COFF_ES_SYMNMLEN	8
#define COFF_ES_SYMENTSZ	18

struct external_syment {
	union {
		char e_name[COFF_ES_SYMNMLEN];
		struct {
			char e_zeroes[4];
			char e_offset[4];
		} e;
	} e;
	char e_value[4];
	char e_scnum[2];
	char e_type[2];
	char e_sclass[1];
	char e_numaux[1];
};

#endif /* !_SH3_COFF_MACHDEP_H_ */

