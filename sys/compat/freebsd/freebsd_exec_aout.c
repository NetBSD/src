/*	$NetBSD: freebsd_exec_aout.c,v 1.2.2.2 2000/12/08 09:08:11 bouyer Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>

#ifndef EXEC_AOUT
#define EXEC_AOUT	/* to get a.out specific stuff */
#endif

#include <compat/freebsd/freebsd_exec.h>

/*
* exec_aout_makecmds(): Check if it's an a.out-format executable.
*
* Given a proc pointer and an exec package pointer, see if the referent
* of the epp is in a.out format.  First check 'standard' magic numbers for
* this architecture.  If that fails, try a cpu-dependent hook.
 *
 * This function, in the former case, or the hook, in the latter, is
 * responsible for creating a set of vmcmds which can be used to build
 * the process's vm space and inserting them into the exec package.
 */

int
exec_freebsd_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	u_long midmag;
	int error = ENOEXEC;
	struct exec *execp = epp->ep_hdr;

	if (epp->ep_hdrvalid < sizeof(struct exec))
		return ENOEXEC;

	midmag = FREEBSD_N_GETMID(*execp) << 16 | FREEBSD_N_GETMAGIC(*execp);

	/* assume FreeBSD's MID_MACHINE and [ZQNO]MAGIC is same as NetBSD's */
	switch (midmag) {
	case (MID_MACHINE << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | QMAGIC:
		error = exec_aout_prep_zmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | NMAGIC:
		error = exec_aout_prep_nmagic(p, epp);
		break;
	case (MID_MACHINE << 16) | OMAGIC:
		error = exec_aout_prep_omagic(p, epp);
		break;
	}
	if (error)
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}
