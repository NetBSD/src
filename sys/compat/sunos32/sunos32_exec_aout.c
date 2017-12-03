/*	$NetBSD: sunos32_exec_aout.c,v 1.11.42.1 2017/12/03 11:36:56 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos32_exec_aout.c,v 1.11.42.1 2017/12/03 11:36:56 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_execfmt.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_exec.h>
#include <compat/sunos/sunos_exec.h>

int
exec_sunos32_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	struct sunos_exec *sunmag = epp->ep_hdr;
	int error = ENOEXEC;

	if (epp->ep_hdrvalid < sizeof(struct sunos_exec))
		return ENOEXEC;
	if (!SUNOS_M_NATIVE(sunmag->a_machtype))
		return (ENOEXEC);

	switch (sunmag->a_magic) {
	case ZMAGIC:
		error = netbsd32_exec_aout_prep_zmagic(l, epp);
		break;
	case NMAGIC:
		error = netbsd32_exec_aout_prep_nmagic(l, epp);
		break;
	case OMAGIC:
		error = netbsd32_exec_aout_prep_omagic(l, epp);
		break;
	}
	return error;
}
