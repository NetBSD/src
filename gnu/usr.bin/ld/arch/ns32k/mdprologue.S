/*
 * Copyright (c) 1994 Matthias Pfaller.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Matthias Pfaller.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 *	$Id: mdprologue.S,v 1.6 1997/07/11 23:39:56 matthias Exp $
 */

/*
 * Run-time link editor entry points for ns32k architecture.
 */

#include <machine/asm.h>

/*
 * rtld_entry(int version, struct crt_ldso *crtp)
 */

ENTRY_NOPROFILE(rtl_entry)
	enter	[],0
	sprd	sb,tos
	addr	_C_LABEL(_GLOBAL_OFFSET_TABLE_)(pc),r0
	lprd	sb,r0			/* PIC function prologue	*/
	movd	12(fp),r0		/* -> crt_ldso structure	*/
	movd	0(r0),r1 		/* base address 		*/
					/* 1st entry in GOT		*/
					/* + ld.so base address 	*/
	addr	0(0(sb))[r1:b],tos	/* gives address of __DYNAMIC	*/
	movd	r0,tos			/* crt_ldso pointer		*/
	movd	8(fp),tos		/* version			*/
	jsr	0(_C_LABEL(rtld)(sb))[r1:b] /* call _rtld function	*/
	lprd	sb,-4(fp)
	exit	[]
	ret	0

/*
 * First call to a procedure generally comes through here for
 * binding.
 */

ENTRY_NOPROFILE(binder_entry)
	save	[r0,r1,r2]		/* preserve scratch registers	*/
					/* save r0 (for cerror) 	*/
					/* save r1 (maybe static chain) */
					/* save r2 (maybe struct addr)	*/
	addr	-6(12(sp)),tos		/* push address of PLT slot	*/
	bsr	_C_LABEL(binder)	/* relocate function address	*/
	cmpqd	0,tos
	movd	r0,12(sp)		/* prepare for jump		*/
	restore	[r0,r1,r2]		/* restore scratch registers	*/
	ret	0			/* jump into function		*/


/* Force cache line corresponding to address argument to be loaded */

	.text
	/* 
	 * Now we have to 0x200-align the text segment.
	 * We know that the a.out header is 0x20 bytes long.
	 * The code so far is less then 0x1e0 bytes. So we
	 * advance "." to address 0x1e0.
	 */

	.org	0x1e0

	/*
	 * We are at address 0x200 now.
	 */

0:	ret	0;			/* 0000 */
ASGLOBAL(_C_LABEL(cinv))
	movd	S_ARG0,r0		/* 0002 */
	andd	0x1f0,r0		/* 0005 */
	jump	0b(pc)[r0:b]		/* 000b */
					/* 000f */

	.align 4,0xa2; ret 0		/* 0010 */
	.align 4,0xa2; ret 0		/* 0020 */
	.align 4,0xa2; ret 0		/* 0030 */
	.align 4,0xa2; ret 0		/* 0040 */
	.align 4,0xa2; ret 0		/* 0050 */
	.align 4,0xa2; ret 0		/* 0060 */
	.align 4,0xa2; ret 0		/* 0070 */
	.align 4,0xa2; ret 0		/* 0080 */
	.align 4,0xa2; ret 0		/* 0090 */
	.align 4,0xa2; ret 0		/* 00a0 */
	.align 4,0xa2; ret 0		/* 00b0 */
	.align 4,0xa2; ret 0		/* 00c0 */
	.align 4,0xa2; ret 0		/* 00d0 */
	.align 4,0xa2; ret 0		/* 00e0 */
	.align 4,0xa2; ret 0		/* 00f0 */
	.align 4,0xa2; ret 0		/* 0100 */
	.align 4,0xa2; ret 0		/* 0110 */
	.align 4,0xa2; ret 0		/* 0120 */
	.align 4,0xa2; ret 0		/* 0130 */
	.align 4,0xa2; ret 0		/* 0140 */
	.align 4,0xa2; ret 0		/* 0150 */
	.align 4,0xa2; ret 0		/* 0160 */
	.align 4,0xa2; ret 0		/* 0170 */
	.align 4,0xa2; ret 0		/* 0180 */
	.align 4,0xa2; ret 0		/* 0190 */
	.align 4,0xa2; ret 0		/* 01a0 */
	.align 4,0xa2; ret 0		/* 01b0 */
	.align 4,0xa2; ret 0		/* 01c0 */
	.align 4,0xa2; ret 0		/* 01d0 */
	.align 4,0xa2; ret 0		/* 01e0 */
	.align 4,0xa2; ret 0		/* 01f0 */
