/*	$NetBSD: freebsd_exec.c,v 1.4.10.1 1999/11/15 00:40:02 fvdl Exp $	*/

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

#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#ifdef EXEC_ELF32
# ifndef ELFSIZE
#  define ELFSIZE 32
# endif /* !ELFSIZE */
# include <sys/exec_elf.h>
#endif /* EXEC_ELF32 */
#include <sys/resourcevar.h>
#include <vm/vm.h>

#include <machine/freebsd_machdep.h>

#include <compat/freebsd/freebsd_exec.h>
#include <compat/freebsd/freebsd_util.h>

#include <compat/freebsd/freebsd_syscall.h>

extern struct sysent freebsd_sysent[];
extern char *freebsd_syscallnames[];

#ifdef EXEC_AOUT
struct emul emul_freebsd_aout = {
	"freebsd",
	NULL,
	freebsd_sendsig,
	FREEBSD_SYS_syscall,
	FREEBSD_SYS_MAXSYSCALL,
	freebsd_sysent,
	freebsd_syscallnames,
	0,
	copyargs,
	freebsd_setregs,
	freebsd_sigcode,
	freebsd_esigcode,
};
#endif /* EXEC_AOUT */

#ifdef EXEC_ELF32
 
struct emul ELFNAMEEND(emul_freebsd) = {
	"freebsd",
	NULL,
	freebsd_sendsig,
	FREEBSD_SYS_syscall,
	FREEBSD_SYS_MAXSYSCALL,
	freebsd_sysent,
	freebsd_syscallnames,
	FREEBSD_ELF_AUX_ARGSIZ,
	ELFNAME(copyargs),
	freebsd_setregs,
	freebsd_sigcode,
	freebsd_esigcode,
};

int
ELFNAME2(freebsd,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
	char *itp;
	Elf_Addr *pos;
{
	int error;
	size_t i;
	size_t phsize;
	Elf_Phdr *ph;
	Elf_Phdr *ephp;
	Elf_Nhdr *np;
	const char *bp;

        static const char wantBrand[] = FREEBSD_ELF_BRAND_STRING;
        static const char wantInterp[] = FREEBSD_ELF_INTERP_PREFIX_STRING;

        /* Insist that the executable have a brand, and that it be "FreeBSD" */
#ifndef EI_BRAND
#define EI_BRAND 8
#endif
        if (eh->e_ident[EI_BRAND] == '\0'
	  || strcmp(&eh->e_ident[EI_BRAND], wantBrand))
		return ENOEXEC;

	i = eh->e_phnum;
	if (i != 0) {
		phsize = i * sizeof(Elf_Phdr);
		ph = (Elf_Phdr *) malloc(phsize, M_TEMP, M_WAITOK);
		if ((error = ELFNAME(read_from)(p, epp->ep_vp, eh->e_phoff,
		    (caddr_t) ph, phsize)) != 0)
			goto bad1;

		for (ephp = ph; i--; ephp++) {
			if (ephp->p_type != PT_INTERP)
				continue;

			/* Check for "legal" intepreter name. */
			if (ephp->p_filesz < sizeof wantInterp)
				goto bad1;

			np = (Elf_Nhdr *) malloc(ephp->p_filesz+1,
			    M_TEMP, M_WAITOK);

			if (((error = ELFNAME(read_from)(p, epp->ep_vp,
			    ephp->p_offset, (caddr_t)np, ephp->p_filesz)) != 0))
				goto bad2;

			if (strncmp((char *)np, wantInterp,
			    sizeof wantInterp - 1))
				goto bad2;

			free(np, M_TEMP);
			break;
		}
		free(ph, M_TEMP);
	}

	if (itp[0]) {
		if ((error = emul_find(p, NULL, freebsd_emul_path,
		    itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &i)) != 0)
			return error;
		free((void *)bp, M_TEMP);
	}
	epp->ep_emul = &ELFNAMEEND(emul_freebsd);
	*pos = ELF_NO_ADDR;
#ifdef DEBUG_FREEBSD_ELF
	printf("freebsd_elf32_probe: returning 0\n");
#endif
	return 0;

bad2:
	free(np, M_TEMP);
bad1:
	free(ph, M_TEMP);
	return ENOEXEC;
}
#endif /* EXEC_ELF32 */


#ifdef EXEC_AOUT
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
	if (error == 0)
		epp->ep_emul = &emul_freebsd_aout;
	else
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}
#endif /* EXEC_AOUT */
