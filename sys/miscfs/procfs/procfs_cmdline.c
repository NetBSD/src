/*	$NetBSD: procfs_cmdline.c,v 1.1 1999/03/12 18:45:40 christos Exp $	*/

/*
 * Copyright (c) 1999 Jaromir Dolecek <dolecek@ics.muni.cz>
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syslimits.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <miscfs/procfs/procfs.h>

/*
 * code for returning process's command line arguments
 */
int
procfs_docmdline(curp, p, pfs, uio)
	struct proc *curp;
	struct proc *p;
	struct pfsnode *pfs;
	struct uio *uio;
{
	struct ps_strings pss;
	char arg[ARG_MAX], *argv;
	int xlen, len, count, i;

	/* don't write; can't work for zombies -- they don't have any stack */
	if (uio->uio_rw != UIO_READ || p->p_stat == SZOMB)
		return EOPNOTSUPP;

	if (copyin(PS_STRINGS, &pss, sizeof(struct ps_strings)))
		return EFAULT;
	if (copyin(pss.ps_argvstr, &argv, sizeof(argv)))
		return EFAULT;
	len = 0;
	count = pss.ps_nargvstr;
	/* don't know how long the argument string is, so let's find out */
	while(count && len < ARG_MAX && copyin(argv+len, &arg[len], 1) == 0) {
		if (len > 0 && arg[len] == '\0') count--;
		len++;
	}
	if (len > 0) len--; /* exclude last NULL */

	xlen = len - uio->uio_offset;
	xlen = imin(xlen, uio->uio_resid);
	if (xlen <= 0) 
		return 0;
	else
		return (uiomove(arg + uio->uio_offset, xlen, uio));
}
