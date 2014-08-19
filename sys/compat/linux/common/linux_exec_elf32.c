/*	$NetBSD: linux_exec_elf32.c,v 1.86.6.1 2014/08/20 00:03:32 tls Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden, Eric Haszlakiewicz and
 * Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_exec_elf32.c,v 1.86.6.1 2014/08/20 00:03:32 tls Exp $");

#ifndef ELFSIZE
/* XXX should die */
#define	ELFSIZE		32
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
#include <sys/stat.h>
#include <sys/kauth.h>
#include <sys/cprng.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <sys/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>

#ifdef DEBUG_LINUX
#define DPRINTF(a)	uprintf a
#else
#define DPRINTF(a)
#endif

#ifdef LINUX_ATEXIT_SIGNATURE
/*
 * On the PowerPC, statically linked Linux binaries are not recognized
 * by linux_signature nor by linux_gcc_signature. Fortunately, thoses
 * binaries features a __libc_atexit ELF section. We therefore assume we
 * have a Linux binary if we find this section.
 */
int
ELFNAME2(linux,atexit_signature)(
	struct lwp *l,
	struct exec_package *epp,
	Elf_Ehdr *eh)
{
	Elf_Shdr *sh;
	size_t shsize;
	u_int shstrndx;
	size_t i;
	static const char signature[] = "__libc_atexit";
	const size_t sigsz = sizeof(signature);
	char tbuf[sizeof(signature)];
	int error;

	/* Load the section header table. */
	shsize = eh->e_shnum * sizeof(Elf_Shdr);
	sh = (Elf_Shdr *) malloc(shsize, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, eh->e_shoff, sh, shsize);
	if (error)
		goto out;

	/* Now let's find the string table. If it does not exist, give up. */
	shstrndx = eh->e_shstrndx;
	if (shstrndx == SHN_UNDEF || shstrndx >= eh->e_shnum) {
		error = ENOEXEC;
		goto out;
	}

	/* Check if any section has the name we're looking for. */
	const off_t stroff = sh[shstrndx].sh_offset;
	for (i = 0; i < eh->e_shnum; i++) {
		Elf_Shdr *s = &sh[i];

		if (s->sh_name + sigsz > sh[shstrndx].sh_size)
			continue;

		error = exec_read_from(l, epp->ep_vp, stroff + s->sh_name, tbuf,
		    sigsz);
		if (error)
			goto out;
		if (!memcmp(tbuf, signature, sigsz)) {
			DPRINTF(("linux_atexit_sig=%s\n", tbuf));
			error = 0;
			goto out;
		}
	}
	error = ENOEXEC;

out:
	free(sh, M_TEMP);
	return (error);
}
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
int
ELFNAME2(linux,gcc_signature)(
	struct lwp *l,
	struct exec_package *epp,
	Elf_Ehdr *eh)
{
	size_t shsize;
	size_t i;
	static const char signature[] = "\0GCC: (GNU) ";
	char tbuf[sizeof(signature) - 1];
	Elf_Shdr *sh;
	int error;

	shsize = eh->e_shnum * sizeof(Elf_Shdr);
	sh = (Elf_Shdr *) malloc(shsize, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, eh->e_shoff, sh, shsize);
	if (error)
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

		error = exec_read_from(l, epp->ep_vp, s->sh_offset, tbuf,
		    sizeof(signature) - 1);
		if (error)
			continue;

		/*
		 * error is 0, if the signatures match we are done.
		 */
		DPRINTF(("linux_gcc_sig: sig=%s\n", tbuf));
		if (!memcmp(tbuf, signature, sizeof(signature) - 1)) {
			error = 0;
			goto out;
		}
	}
	error = ENOEXEC;

out:
	free(sh, M_TEMP);
	return (error);
}
#endif

#ifdef LINUX_DEBUGLINK_SIGNATURE
/*
 * Look for a .gnu_debuglink, specific to x86_64 interpreter
 */
int
ELFNAME2(linux,debuglink_signature)(struct lwp *l, struct exec_package *epp, Elf_Ehdr *eh)
{
	Elf_Shdr *sh;
	size_t shsize;
	u_int shstrndx;
	size_t i;
	static const char signature[] = ".gnu_debuglink";
	const size_t sigsz = sizeof(signature);
	char tbuf[sizeof(signature)];
	int error;

	/* Load the section header table. */
	shsize = eh->e_shnum * sizeof(Elf_Shdr);
	sh = (Elf_Shdr *) malloc(shsize, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, eh->e_shoff, sh, shsize);
	if (error)
		goto out;

	/* Now let's find the string table. If it does not exist, give up. */
	shstrndx = eh->e_shstrndx;
	if (shstrndx == SHN_UNDEF || shstrndx >= eh->e_shnum) {
		error = ENOEXEC;
		goto out;
	}

	/* Check if any section has the name we're looking for. */
	const off_t stroff = sh[shstrndx].sh_offset;
	for (i = 0; i < eh->e_shnum; i++) {
		Elf_Shdr *s = &sh[i];

		if (s->sh_name + sigsz > sh[shstrndx].sh_size)
			continue;

		error = exec_read_from(l, epp->ep_vp, stroff + s->sh_name, tbuf,
		    sigsz);
		if (error)
			goto out;
		if (!memcmp(tbuf, signature, sigsz)) {
			DPRINTF(("linux_debuglink_sig=%s\n", tbuf));
			error = 0;
			goto out;
		}
	}
	error = ENOEXEC;

out:
	free(sh, M_TEMP);
	return (error);
}
#endif

int
ELFNAME2(linux,signature)(struct lwp *l, struct exec_package *epp, Elf_Ehdr *eh, char *itp)
{
	size_t i;
	Elf_Phdr *ph;
	size_t phsize;
	int error;
	static const char linux[] = "Linux";

	if (eh->e_ident[EI_OSABI] == 3 ||
	    memcmp(&eh->e_ident[EI_ABIVERSION], linux, sizeof(linux)) == 0)
		return 0;

	phsize = eh->e_phnum * sizeof(Elf_Phdr);
	ph = (Elf_Phdr *)malloc(phsize, M_TEMP, M_WAITOK);
	error = exec_read_from(l, epp->ep_vp, eh->e_phoff, ph, phsize);
	if (error)
		goto out;

	for (i = 0; i < eh->e_phnum; i++) {
		Elf_Phdr *ephp = &ph[i];
		Elf_Nhdr *np;
		u_int32_t *abi;

		if (ephp->p_type != PT_NOTE ||
		    ephp->p_filesz > 1024 ||
		    ephp->p_filesz < sizeof(Elf_Nhdr) + 20)
			continue;

		np = (Elf_Nhdr *)malloc(ephp->p_filesz, M_TEMP, M_WAITOK);
		error = exec_read_from(l, epp->ep_vp, ephp->p_offset, np,
		    ephp->p_filesz);
		if (error)
			goto next;

		if (np->n_type != ELF_NOTE_TYPE_ABI_TAG ||
		    np->n_namesz != ELF_NOTE_ABI_NAMESZ ||
		    np->n_descsz != ELF_NOTE_ABI_DESCSZ ||
		    memcmp((void *)(np + 1), ELF_NOTE_ABI_NAME,
		    ELF_NOTE_ABI_NAMESZ))
			goto next;

		/* Make sure the OS is Linux. */
		abi = (u_int32_t *)((char *)np + sizeof(Elf_Nhdr) +
		    np->n_namesz);
		if (abi[0] == ELF_NOTE_ABI_OS_LINUX)
			error = 0;
		else
			error = ENOEXEC;
		free(np, M_TEMP);
		goto out;

	next:
		free(np, M_TEMP);
		continue;
	}

	/* Check for certain interpreter names. */
	if (itp) {
		if (!strncmp(itp, "/lib/ld-linux", 13) ||
#if (ELFSIZE == 64)
		    !strncmp(itp, "/lib64/ld-linux", 15) ||
#endif
		    !strncmp(itp, "/lib/ld.so.", 11))
			error = 0;
		else
			error = ENOEXEC;
		goto out;
	}

	error = ENOEXEC;
out:
	free(ph, M_TEMP);
	return (error);
}

int
ELFNAME2(linux,probe)(struct lwp *l, struct exec_package *epp, void *eh,
    char *itp, vaddr_t *pos)
{
	int error;

	if (((error = ELFNAME2(linux,signature)(l, epp, eh, itp)) != 0) &&
#ifdef LINUX_GCC_SIGNATURE
	    ((error = ELFNAME2(linux,gcc_signature)(l, epp, eh)) != 0) &&
#endif
#ifdef LINUX_ATEXIT_SIGNATURE
	    ((error = ELFNAME2(linux,atexit_signature)(l, epp, eh)) != 0) &&
#endif
#ifdef LINUX_DEBUGLINK_SIGNATURE
	    ((error = ELFNAME2(linux,debuglink_signature)(l, epp, eh)) != 0) &&
#endif
	    1) {
			DPRINTF(("linux_probe: returning %d\n", error));
			return error;
	}

	if (itp) {
		if ((error = emul_find_interp(l, epp, itp)))
			return (error);
	}
	epp->ep_flags |= EXEC_FORCEAUX;
	DPRINTF(("linux_probe: returning 0\n"));
	return 0;
}

#ifndef LINUX_MACHDEP_ELF_COPYARGS
/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
ELFNAME2(linux,copyargs)(struct lwp *l, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	size_t len;
	AuxInfo ai[LINUX_ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;
	int error;
	struct vattr *vap;
	uint32_t randbytes[4];

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	a = ai;

	/*
	 * Push extra arguments used by glibc on the stack.
	 */

	a->a_type = AT_PAGESZ;
	a->a_v = PAGE_SIZE;
	a++;

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

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		exec_free_emul_arg(pack);
	}

	/* Linux-specific items */
	a->a_type = LINUX_AT_CLKTCK;
	a->a_v = hz;
	a++;

	vap = pack->ep_vap;

	a->a_type = LINUX_AT_UID;
	a->a_v = kauth_cred_getuid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_EUID;
	if (vap->va_mode & S_ISUID)
		a->a_v = vap->va_uid;
	else
		a->a_v = kauth_cred_geteuid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_GID;
	a->a_v = kauth_cred_getgid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_EGID;
	if (vap->va_mode & S_ISGID)
		a->a_v = vap->va_gid;
	else
		a->a_v = kauth_cred_getegid(l->l_cred);
	a++;

	a->a_type = LINUX_AT_RANDOM;
	a->a_v = (Elf_Addr)*stackp;
	a++;

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	randbytes[0] = cprng_strong32();
	randbytes[1] = cprng_strong32();
	randbytes[2] = cprng_strong32();
	randbytes[3] = cprng_strong32();

	len = sizeof(randbytes);
	if ((error = copyout(randbytes, *stackp, len)) != 0)
		return error;
	*stackp += len;

	len = (a - ai) * sizeof(AuxInfo);
	KASSERT(len <= LINUX_ELF_AUX_ENTRIES * sizeof(AuxInfo));
	if ((error = copyout(ai, *stackp, len)) != 0)
		return error;
	*stackp += len;

	return 0;
}
#endif /* !LINUX_MACHDEP_ELF_COPYARGS */
