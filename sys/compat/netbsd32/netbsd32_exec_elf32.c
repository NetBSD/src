/*	$NetBSD: netbsd32_exec_elf32.c,v 1.2.2.2 2000/12/08 09:08:34 bouyer Exp $	*/
/*	from: NetBSD: exec_aout.c,v 1.15 1996/09/26 23:34:46 cgd Exp */

/*
 * Copyright (c) 1998 Matthew R. Green.
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

#define	ELFSIZE		32

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>

#include <machine/frame.h>
#include <machine/netbsd32_machdep.h>

int netbsd32_copyinargs __P((struct exec_package *, struct ps_strings *, 
			     void *, size_t, const void *, const void *));

extern int ELFNAME2(netbsd,signature) __P((struct proc *, struct exec_package *,
    Elf_Ehdr *));

int
ELFNAME2(netbsd32,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp;
	vaddr_t *pos;
{
	int error;
	size_t i;
	const char *bp;

	if ((error = ELFNAME2(netbsd,signature)(p, epp, eh)) != 0)
		return error;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path,
				       itp, &bp, 0)) && 
		    (error = emul_find(p, NULL, "", itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &i)) != 0)
			return error;
		free((void *)bp, M_TEMP);
	}
	epp->ep_flags |= EXEC_32;
	*(Elf_Addr *)pos = ELFDEFNNAME(NO_ADDR);
	return 0;
}

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
void *
netbsd32_elf32_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	size_t len;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;

	stack = netbsd32_copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return NULL;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = NBPG;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return stack;
}
