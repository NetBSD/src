/*	$NetBSD: dot_init.h,v 1.4 2003/03/24 14:32:57 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2001 Ross Harvey
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

/*
 * The SH5 toolchain generates a bogus GOT access in _init() if we use
 * the traditional approach as used by other ports.
 * Work-around this using a totally gross hack for now.
 */
#define	INIT_FALLTHRU_DECL
#define	FINI_FALLTHRU_DECL

#define	INIT_FALLTHRU()	\
	__asm __volatile("	addi	r15, -16, r15		\n"\
			"	st.q	r15, 0, r18		\n"\
			"	gettr	tr0, r18		\n"\
			"	st.q	r15, 8, r18		\n"\
			"	pt/u	init_fallthru, tr0	\n"\
			"	gettr	tr0, r18		\n"\
			"	ori	r18, 1, r18		\n"\
			"	ptabs/l	r18, tr0		\n"\
			"	blink	tr0, r18		\n"\
			"	ld.q	r15, 8, r18		\n"\
			"	ptabs/u	r18, tr0		\n"\
			"	ld.q	r15, 0, r18		\n"\
			"	addi	r15, 16, r15")
#define	FINI_FALLTHRU()	\
	__asm __volatile("	addi	r15, -16, r15		\n"\
			"	st.q	r15, 0, r18		\n"\
			"	gettr	tr0, r18		\n"\
			"	st.q	r15, 8, r18		\n"\
			"	pt/u	fini_fallthru, tr0	\n"\
			"	gettr	tr0, r18		\n"\
			"	ori	r18, 1, r18		\n"\
			"	ptabs/l	r18, tr0		\n"\
			"	blink	tr0, r18		\n"\
			"	ld.q	r15, 8, r18		\n"\
			"	ptabs/u	r18, tr0		\n"\
			"	ld.q	r15, 0, r18		\n"\
			"	addi	r15, 16, r15")

#define	MD_SECTION_PROLOGUE(sect, entry_pt)		\
		__asm (					\
		".section "#sect",\"ax\",@progbits	\n"\
		".align 5				\n"\
		".type "#entry_pt",@function		\n"\
		#entry_pt":				\n"\
		"	addi	r15, -16, r15		\n"\
		"	st.q	r15, 0, r14		\n"\
		"	st.q	r15, 8, r18		\n"\
		"	add	r15, r63, r14		\n"\
		"	nop; nop; nop; nop		\n"\
		"	/* fall thru */			\n"\
		".previous")

#define	MD_SECTION_EPILOGUE(sect)			\
		__asm (					\
		".section "#sect",\"ax\",@progbits	\n"\
		"	ld.q	r15, 8, r18		\n"\
		"	ld.q	r15, 0, r14		\n"\
		"	ptabs/l	r18, tr0		\n"\
		"	addi	r15, 16, r15		\n"\
		"	blink	tr0, r63		\n"\
		".previous")

#if __GNUC__ < 3
#define	MD_INIT_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.init, _init_fallthru)
#define	MD_FINI_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.fini, _fini_fallthru)
#else
#define	MD_INIT_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.init, init_fallthru)
#define	MD_FINI_SECTION_PROLOGUE MD_SECTION_PROLOGUE(.fini, fini_fallthru)
#endif

#define	MD_INIT_SECTION_EPILOGUE MD_SECTION_EPILOGUE(.init)
#define	MD_FINI_SECTION_EPILOGUE MD_SECTION_EPILOGUE(.fini)
