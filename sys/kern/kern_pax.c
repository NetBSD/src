/* $NetBSD: kern_pax.c,v 1.2.2.1 2006/06/19 04:07:15 chap Exp $ */

/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include "opt_pax.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/exec_elf.h>
#include <uvm/uvm_extern.h>
#include <sys/pax.h>
#include <sys/sysctl.h>

int pax_mprotect_enabled = 1;
int pax_mprotect_global = PAX_MPROTECT;

SYSCTL_SETUP(sysctl_security_pax_setup, "sysctl security.pax setup")
{
	const struct sysctlnode *rnode = NULL;

        sysctl_createv(clog, 0, NULL, &rnode,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "security", NULL,
                       NULL, 0, NULL, 0,
                       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "pax",
		       SYSCTL_DESCR("PaX (exploit mitigation) features."),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "mprotect",
		       SYSCTL_DESCR("mprotect(2) W^X restrictions."),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("Restrictions enabled."),
		       NULL, 0, &pax_mprotect_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "global_protection",
		       SYSCTL_DESCR("When enabled, unless explicitly "
				    "specified, apply restrictions to"
				    "all processes."),
		       NULL, 0, &pax_mprotect_global, 0,
		       CTL_CREATE, CTL_EOL);
}

void
pax_mprotect_adjust(struct lwp *l, int f)
{
	if (!pax_mprotect_enabled)
		return;

	if (f & PF_PAXMPROTECT)
		l->l_proc->p_flag |= P_PAXMPROTECT;
	if (f & PF_PAXNOMPROTECT)
		l->l_proc->p_flag |= P_PAXNOMPROTECT;
}

void
pax_mprotect(struct lwp *l, vm_prot_t *prot, vm_prot_t *maxprot)
{
	if (!pax_mprotect_enabled ||
	    (pax_mprotect_global && (l->l_proc->p_flag & P_PAXNOMPROTECT)) ||
	    (!pax_mprotect_global && !(l->l_proc->p_flag & P_PAXMPROTECT)))
		return;

	if ((*prot & (VM_PROT_WRITE|VM_PROT_EXECUTE)) != VM_PROT_EXECUTE) {
		*prot &= ~VM_PROT_EXECUTE;
		*maxprot &= ~VM_PROT_EXECUTE;
	} else {
		*prot &= ~VM_PROT_WRITE;
		*maxprot &= ~VM_PROT_WRITE;
	}
}
