/*	$NetBSD: macho_machdep.h,v 1.2.4.2 2001/08/03 04:11:45 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef _I386_MACHO_MACHDEP_H_
#define	_I386_MACHO_MACHDEP_H_

#define MACHO_I386_THREAD_STATE	-1
struct exec_macho_i386_thread_state {
	unsigned int	eax;
	unsigned int	ebx;
	unsigned int	ecx;
	unsigned int	edx;
	unsigned int	edi;
	unsigned int	esi;
	unsigned int	ebp;
	unsigned int	esp;
	unsigned int	ss;
	unsigned int	eflags;
	unsigned int	eip;
	unsigned int	cs;
	unsigned int	ds;
	unsigned int	es;
	unsigned int	fs;
	unsigned int	gs;
};

#define MACHO_I386_NEW_THREAD_STATE	1
struct exec_macho_i386_saved_state {
	unsigned int	gs;
	unsigned int	fs;
	unsigned int	es;
	unsigned int	ds;
	unsigned int	edi;
	unsigned int	esi;
	unsigned int	ebp;
	unsigned int	esp;
	unsigned int	ebx;
	unsigned int	edx;
	unsigned int	ecx;
	unsigned int	eax;
	unsigned int	trapno;
	unsigned int	err;
	unsigned int	eip;
	unsigned int	cs;
	unsigned int	efl;
	unsigned int	uesp;
	unsigned int	ss;
	struct vm86_segs {
		unsigned int es;
		unsigned int ds;
		unsigned int fs;
		unsigned int gs;
	} vm86_segs; 
#define MACHO_I386_SAVED_ARGV_COUNT	7
	unsigned int	argv_status;
	unsigned int	argv[MACHO_I386_SAVED_ARGV_COUNT];
};

#define MACHO_MACHDEP_CASES		\
	case MACHO_CPU_TYPE_I386:	\
		goto done;

#endif /* !_I386_MACHO_MACHDEP_H_ */
