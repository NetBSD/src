/*	$NetBSD: genassym.c,v 1.1 1996/09/13 00:02:39 jtk Exp $	*/

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)genassym.c	5.11 (Berkeley) 5/10/91
 */

#include <sys/types.h>
#include <i386/include/apmvar.h>

#include <stdio.h>
#include <stddef.h>

int main __P((void));

int
main()
{

#define	def(N,V)	printf("#define\t%s %d\n", N, V)
#define	off(N,S,M)	def(N, (int)offsetof(S, M))

	off("APM_CODE32", struct apm_connect_info, apm_code32_seg_base);
	off("APM_CODE16", struct apm_connect_info, apm_code16_seg_base);
	off("APM_DATA", struct apm_connect_info, apm_data_seg_base);
	off("APM_CODE32_LEN", struct apm_connect_info, apm_code32_seg_len);
	off("APM_DATA_LEN", struct apm_connect_info, apm_data_seg_len);
	off("APM_ENTRY", struct apm_connect_info, apm_entrypt);
	off("APM_DETAIL", struct apm_connect_info, apm_detail);
	def("APM_SIZE", sizeof(struct apm_connect_info));
	off("APMREG_AX", struct apmregs, ax);
	off("APMREG_BX", struct apmregs, bx);
	off("APMREG_CX", struct apmregs, cx);
	off("APMREG_DX", struct apmregs, dx);
	off("APMREG_SI", struct apmregs, si);
	off("APMREG_DI", struct apmregs, di);
	off("APMREG_FLAGS", struct apmregs, flags);
	exit(0);
}
