/*	$NetBSD: sysident.h,v 1.1 1997/03/21 05:39:43 cgd Exp $	*/

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
 */

#define	__S(x)	__STRING(x)
__asm("
	.section \".note.netbsd.ident\", \"a\"
	.p2align 2

	# NetBSD note: OS version
	.long	7				# name size
	.long	4				# desc size
	.long	" __S(ELF_NOTE_NETBSD_TYPE_OSVERSION) " # type
	.ascii \"NetBSD\\0\\0\"			# name (padded to % 4 == 0)
	.long	" __S(NetBSD) "			# desc (padded to % 4 == 0)

	# NetBSD note: emulation name
	.long	7				# name size
	.long	7				# desc size
	.long	" __S(ELF_NOTE_NETBSD_TYPE_EMULNAME) " # type
	.ascii	\"NetBSD\\0\\0\"		# name (padded to % 4 == 0)
	.ascii	\"netbsd\\0\\0\"		# desc (padded to % 4 == 0)
");
