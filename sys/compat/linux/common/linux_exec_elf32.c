/*	$NetBSD: linux_exec_elf32.c,v 1.42.2.3 2000/12/08 09:08:27 bouyer Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden and Eric Haszlakiewicz.
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

/*
 * based on exec_aout.c, sunos_exec.c and svr4_exec.c
 */

#ifndef ELFSIZE
#define	ELFSIZE		32				/* XXX should die */
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

static int ELFNAME2(linux,signature) __P((struct proc *, struct exec_package *,
	Elf_Ehdr *));
#ifdef LINUX_GCC_SIGNATURE
static int ELFNAME2(linux,gcc_signature) __P((struct proc *p,
	struct exec_package *, Elf_Ehdr *));
#endif

#ifdef LINUX_GCC_SIGNATURE
/*
 * Take advantage of the fact that all the linux binaries are compiled
 * with gcc, and gcc sticks in the comment field a signature. Note that
 * on SVR4 binaries, the gcc signature will follow the OS name signature,
 * that will not be a problem. We don't bother to read in the string table,
 * but we check all the progbits headers.
 *
 * XXX This only works in the i386.  On the alpha (at least)
 * XXX we have the same gcc signature which incorrectly identifies
 * XXX NetBSD binaries as Linux.
 */
static int
ELFNAME2(linux,gcc_signature)(p, epp, eh)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
{
	size_t shsize = sizeof(Elf_Shdr) * eh->e_shnum;
	size_t i;
	static const char signature[] = "\0GCC: (GNU) ";
	char buf[sizeof(signature) - 1];
	Elf_Shdr *sh;
	int error;

	error = ENOEXEC;
	sh = (Elf_Shdr *) malloc(shsize, M_TEMP, M_WAITOK);

	if ((error = ELFNAME(read_from)(p, epp->ep_vp, eh->e_shoff,
	    (caddr_t) sh, shsize)) != 0)
		goto out;

	for (i = 0; i < eh->e_shnum; i++) {
		Elf_Shdr *s = &sh[i];

		/*
		 * Identify candidates for the comment header;
		 * Header cannot have a load address, or flags and
		 * it must be large enough.
		 */
		if (s->sh_type != SHT_PROGBITS ||
		    s->sh_addr != 0 ||
		    s->sh_flags != 0 ||
		    s->sh_size < sizeof(signature) - 1)
			continue;

		if ((error = ELFNAME(read_from)(p, epp->ep_vp, s->sh_offset,
		    (caddr_t) buf, sizeof(signature) - 1)) != 0)
			goto out;

		/*
		 * error is 0, if the signatures match we are done.
		 */
#ifdef DEBUG_LINUX
		printf("linux_gcc_sig: sig=%s\n", buf);
#endif
		if (memcmp(buf, signature, sizeof(signature) - 1) == 0)
			goto out;
	}

out:
	free(sh, M_TEMP);
#ifdef DEBUG_LINUX
	printf("linux_gcc_sig: returning %d\n", error);
#endif
	return error;
}
#endif

static int
ELFNAME2(linux,signature)(p, epp, eh)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
{
	size_t i;
	Elf_Phdr *ph;
	Elf_Nhdr *notep;
	size_t phsize;
	int error = ENOEXEC;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	if ((error = ELFNAME(read_from)(p, epp->ep_vp, eh->e_phoff,
					(caddr_t) ph, phsize)) != 0)
		goto out1;

	for (i = 0; i < eh->e_phnum; i++) {
		Elf_Phdr *ephp = &ph[i];
		u_int32_t ostype;

		if (ephp->p_type != PT_INTERP /* XAX PT_NOTE */
#if 0
		    || ephp->p_flags != 0
		    || ephp->p_filesz < sizeof(Elf_Nhdr))
#endif
		    )
			continue;

		notep = (Elf_Nhdr *)malloc(ephp->p_filesz+1, M_TEMP, M_WAITOK);
		if ((error = ELFNAME(read_from)(p, epp->ep_vp, ephp->p_offset,
					(caddr_t)notep, ephp->p_filesz)) != 0)
			goto out3;

		/* Check for "linux" in the intepreter name. */
		if (ephp->p_filesz < 8 + 5) {
			error = ENOEXEC;
			goto out3;
		}
#ifdef DEBUG_LINUX
		printf("linux_signature: interp=%s\n", (char *)notep);
#endif
		if (strncmp(&((char *)notep)[8], "linux", 5) == 0 ||
		    strncmp((char *)notep, "/lib/ld.so.", 11) == 0) {
			error = 0;
			goto out3;
		}

		goto out2;

		/* XXX XAX Should handle NETBSD_TYPE_EMULNAME */
		if (notep->n_type != ELF_NOTE_TYPE_OSVERSION) {
			free(notep, M_TEMP);
			continue;
		}

		/* Check the name and description sizes. */
		if (notep->n_namesz != ELF_NOTE_GNU_NAMESZ ||
		    notep->n_descsz != ELF_NOTE_GNU_DESCSZ)
			goto out2;

		/* Is the name "GNU\0"? */
		if (memcmp((notep + sizeof(Elf_Nhdr)),
			   ELF_NOTE_GNU_NAME, ELF_NOTE_GNU_NAMESZ))
			goto out2;

		/* Make sure the OS is Linux */
		ostype = (u_int32_t)(*((u_int32_t *)notep + sizeof(Elf_Nhdr) +
		    notep->n_namesz)) & ELF_NOTE_GNU_OSMASK;
		if (ostype != ELF_NOTE_GNU_OSLINUX)
			goto out2;

		/* All checks succeeded. */
		error = 0;
		goto out3;
	}

	error = ENOEXEC;

out1:
	free(ph, M_TEMP);
#ifdef DEBUG_LINUX
	printf("linux_signature: out1=%d\n", error);
#endif
	return error;

out2:
	error = ENOEXEC;
out3:
	free(notep, M_TEMP);
	free(ph, M_TEMP);
#ifdef DEBUG_LINUX
	printf("linux_signature: out2,3=%d\n", error);
#endif
	return error;
}

int
ELFNAME2(linux,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp;
	vaddr_t *pos;
{
	const char *bp;
	int error;
	size_t len;

	if ((error = ELFNAME2(linux,signature)(p, epp, eh)) != 0)
#ifdef LINUX_GCC_SIGNATURE
		if ((error = ELFNAME2(linux,gcc_signature)(p, epp, eh)) != 0)
			return error;
#else
		return error;
#endif

	if (itp[0]) {
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path,
		    itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	*pos = ELF_NO_ADDR;
#ifdef DEBUG_LINUX
	printf("linux_probe: returning 0\n");
#endif
	return 0;
}

