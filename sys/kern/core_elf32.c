/*	$NetBSD: core_elf32.c,v 1.22.6.2 2006/06/01 22:38:06 kardel Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * core_elf32.c/core_elf64.c: Support for the Elf32/Elf64 core file format.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: core_elf32.c,v 1.22.6.2 2006/06/01 22:38:06 kardel Exp $");

/* If not included by core_elf64.c, ELFSIZE won't be defined. */
#ifndef ELFSIZE
#define	ELFSIZE		32
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/ptrace.h>
#include <sys/malloc.h>
#include <sys/kauth.h>

#include <machine/reg.h>

#include <uvm/uvm_extern.h>

struct countsegs_state {
	int	npsections;
};

static int	ELFNAMEEND(coredump_countsegs)(struct proc *, void *,
		    struct uvm_coredump_state *);

struct writesegs_state {
	Elf_Phdr *psections;
	off_t	secoff;
};

static int	ELFNAMEEND(coredump_writeseghdrs)(struct proc *, void *,
		    struct uvm_coredump_state *);

static int	ELFNAMEEND(coredump_notes)(struct proc *, struct lwp *, void *,
		    size_t *);
static int	ELFNAMEEND(coredump_note)(struct proc *, struct lwp *, void *,
		    size_t *);

#define	ELFROUNDSIZE	4	/* XXX Should it be sizeof(Elf_Word)? */
#define	elfround(x)	roundup((x), ELFROUNDSIZE)

#define elf_process_read_regs	CONCAT(process_read_regs, ELFSIZE)
#define elf_process_read_fpregs	CONCAT(process_read_fpregs, ELFSIZE)
#define elf_reg			CONCAT(process_reg, ELFSIZE)
#define elf_fpreg		CONCAT(process_fpreg, ELFSIZE)

int
ELFNAMEEND(coredump)(struct lwp *l, void *cookie)
{
	struct proc *p;
	Elf_Ehdr ehdr;
	Elf_Phdr phdr, *psections;
	struct countsegs_state cs;
	struct writesegs_state ws;
	off_t notestart, secstart, offset;
	size_t notesize;
	int error, i;

	psections = NULL;
	p = l->l_proc;
	/*
	 * We have to make a total of 3 passes across the map:
	 *
	 *	1. Count the number of map entries (the number of
	 *	   PT_LOAD sections).
	 *
	 *	2. Write the P-section headers.
	 *
	 *	3. Write the P-sections.
	 */

	/* Pass 1: count the entries. */
	cs.npsections = 0;
	error = uvm_coredump_walkmap(p, NULL,
	    ELFNAMEEND(coredump_countsegs), &cs);
	if (error)
		goto out;

	/* Count the PT_NOTE section. */
	cs.npsections++;

	/* Get the size of the notes. */
	error = ELFNAMEEND(coredump_notes)(p, l, NULL, &notesize);
	if (error)
		goto out;

	memset(&ehdr.e_ident[EI_PAD], 0, sizeof(ehdr.e_ident) - EI_PAD);
	memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
#if ELFSIZE == 32
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
#elif ELFSIZE == 64
	ehdr.e_ident[EI_CLASS] = ELFCLASS64;
#endif
	ehdr.e_ident[EI_DATA] = ELFDEFNNAME(MACHDEP_ENDIANNESS);
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	/* XXX Should be the OSABI/ABI version of the executable. */
	ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
	ehdr.e_ident[EI_ABIVERSION] = 0;

	ehdr.e_type = ET_CORE;
	/* XXX This should be the e_machine of the executable. */
	ehdr.e_machine = ELFDEFNNAME(MACHDEP_ID);
	ehdr.e_version = EV_CURRENT;
	ehdr.e_entry = 0;
	ehdr.e_phoff = sizeof(ehdr);
	ehdr.e_shoff = 0;
	ehdr.e_flags = 0;
	ehdr.e_ehsize = sizeof(ehdr);
	ehdr.e_phentsize = sizeof(Elf_Phdr);
	ehdr.e_phnum = cs.npsections;
	ehdr.e_shentsize = 0;
	ehdr.e_shnum = 0;
	ehdr.e_shstrndx = 0;

	/* Write out the ELF header. */
	error = coredump_write(cookie, UIO_SYSSPACE, &ehdr, sizeof(ehdr));
	if (error)
		goto out;

	offset = sizeof(ehdr);

	notestart = offset + sizeof(phdr) * cs.npsections;
	secstart = notestart + notesize;

	psections = malloc(cs.npsections * sizeof(Elf_Phdr),
	    M_TEMP, M_WAITOK|M_ZERO);

	/* Pass 2: now write the P-section headers. */
	ws.secoff = secstart;
	ws.psections = psections;
	error = uvm_coredump_walkmap(p, cookie,
	    ELFNAMEEND(coredump_writeseghdrs), &ws);
	if (error)
		goto out;

	/* Write out the PT_NOTE header. */
	ws.psections->p_type = PT_NOTE;
	ws.psections->p_offset = notestart;
	ws.psections->p_vaddr = 0;
	ws.psections->p_paddr = 0;
	ws.psections->p_filesz = notesize;
	ws.psections->p_memsz = 0;
	ws.psections->p_flags = PF_R;
	ws.psections->p_align = ELFROUNDSIZE;

	error = coredump_write(cookie, UIO_SYSSPACE, psections,
	    cs.npsections * sizeof(Elf_Phdr));
	if (error)
		goto out;

#ifdef DIAGNOSTIC
	offset += cs.npsections * sizeof(Elf_Phdr);
	if (offset != notestart)
		panic("coredump: offset %lld != notestart %lld",
		    (long long) offset, (long long) notestart);
#endif

	/* Write out the notes. */
	error = ELFNAMEEND(coredump_notes)(p, l, cookie, &notesize);
	if (error)
		goto out;

#ifdef DIAGNOSTIC
	offset += notesize;
	if (offset != secstart)
		panic("coredump: offset %lld != secstart %lld",
		    (long long) offset, (long long) secstart);
#endif

	/* Pass 3: finally, write the sections themselves. */
	for (i = 0; i < cs.npsections - 1; i++) {
		if (psections[i].p_filesz == 0)
			continue;

#ifdef DIAGNOSTIC
		if (offset != psections[i].p_offset)
			panic("coredump: offset %lld != p_offset[%d] %lld",
			    (long long) offset, i,
			    (long long) psections[i].p_filesz);
#endif

		error = coredump_write(cookie, UIO_USERSPACE,
		    (void *)(vaddr_t)psections[i].p_vaddr,
		    psections[i].p_filesz);
		if (error)
			goto out;

#ifdef DIAGNOSTIC
		offset += psections[i].p_filesz;
#endif
	}

  out:
	if (psections)
		free(psections, M_TEMP);
	return (error);
}

static int
ELFNAMEEND(coredump_countsegs)(struct proc *p, void *iocookie,
    struct uvm_coredump_state *us)
{
	struct countsegs_state *cs = us->cookie;

	cs->npsections++;
	return (0);
}

static int
ELFNAMEEND(coredump_writeseghdrs)(struct proc *p, void *iocookie,
    struct uvm_coredump_state *us)
{
	struct writesegs_state *ws = us->cookie;
	Elf_Phdr phdr;
	vsize_t size, realsize;
	vaddr_t end;
	int error;

	size = us->end - us->start;
	realsize = us->realend - us->start;
	end = us->realend;

	while (realsize > 0) {
		long buf[1024 / sizeof(long)];
		size_t slen = realsize > sizeof(buf) ? sizeof(buf) : realsize;
		const long *ep;
		int i;

		end -= slen;
		if ((error = copyin_proc(p, (void *)end, buf, slen)) != 0)
			return error;

		ep = (const long *) &buf[slen / sizeof(buf[0])];
		for (i = 0, ep--; buf <= ep; ep--, i++) {
			if (*ep)
				break;
		}
		realsize -= i * sizeof(buf[0]);
		if (i * sizeof(buf[0]) < slen)
			break;
	}

	phdr.p_type = PT_LOAD;
	phdr.p_offset = ws->secoff;
	phdr.p_vaddr = us->start;
	phdr.p_paddr = 0;
	phdr.p_filesz = realsize;
	phdr.p_memsz = size;
	phdr.p_flags = 0;
	if (us->prot & VM_PROT_READ)
		phdr.p_flags |= PF_R;
	if (us->prot & VM_PROT_WRITE)
		phdr.p_flags |= PF_W;
	if (us->prot & VM_PROT_EXECUTE)
		phdr.p_flags |= PF_X;
	phdr.p_align = PAGE_SIZE;

	ws->secoff += phdr.p_filesz;
	*ws->psections++ = phdr;

	return (0);
}

static int
ELFNAMEEND(coredump_notes)(struct proc *p, struct lwp *l,
    void *iocookie, size_t *sizep)
{
	struct netbsd_elfcore_procinfo cpi;
	Elf_Nhdr nhdr;
	size_t size, notesize;
	int error;
	struct lwp *l0;

	size = 0;

	/* First, write an elfcore_procinfo. */
	notesize = sizeof(nhdr) + elfround(sizeof(ELF_NOTE_NETBSD_CORE_NAME)) +
	    elfround(sizeof(cpi));
	if (iocookie) {
		cpi.cpi_version = NETBSD_ELFCORE_PROCINFO_VERSION;
		cpi.cpi_cpisize = sizeof(cpi);
		cpi.cpi_signo = p->p_sigctx.ps_signo;
		cpi.cpi_sigcode = p->p_sigctx.ps_code;
		cpi.cpi_siglwp = p->p_sigctx.ps_lwp;

		memcpy(&cpi.cpi_sigpend, &p->p_sigctx.ps_siglist,
		    sizeof(cpi.cpi_sigpend));
		memcpy(&cpi.cpi_sigmask, &p->p_sigctx.ps_sigmask,
		    sizeof(cpi.cpi_sigmask));
		memcpy(&cpi.cpi_sigignore, &p->p_sigctx.ps_sigignore,
		    sizeof(cpi.cpi_sigignore));
		memcpy(&cpi.cpi_sigcatch, &p->p_sigctx.ps_sigcatch,
		    sizeof(cpi.cpi_sigcatch));

		cpi.cpi_pid = p->p_pid;
		cpi.cpi_ppid = p->p_pptr->p_pid;
		cpi.cpi_pgrp = p->p_pgid;
		cpi.cpi_sid = p->p_session->s_sid;

		cpi.cpi_ruid = kauth_cred_getuid(p->p_cred);
		cpi.cpi_euid = kauth_cred_geteuid(p->p_cred);
		cpi.cpi_svuid = kauth_cred_getsvuid(p->p_cred);

		cpi.cpi_rgid = kauth_cred_getgid(p->p_cred);
		cpi.cpi_egid = kauth_cred_getegid(p->p_cred);
		cpi.cpi_svgid = kauth_cred_getsvgid(p->p_cred);

		cpi.cpi_nlwps = p->p_nlwps;
		(void)strncpy(cpi.cpi_name, p->p_comm, sizeof(cpi.cpi_name));
		cpi.cpi_name[sizeof(cpi.cpi_name) - 1] = '\0';

		nhdr.n_namesz = sizeof(ELF_NOTE_NETBSD_CORE_NAME);
		nhdr.n_descsz = sizeof(cpi);
		nhdr.n_type = ELF_NOTE_NETBSD_CORE_PROCINFO;

		error = ELFNAMEEND(coredump_writenote)(p, iocookie, &nhdr,
		    ELF_NOTE_NETBSD_CORE_NAME "\0\0\0", &cpi);
		if (error)
			return (error);
	}

	size += notesize;

	/* XXX Add hook for machdep per-proc notes. */

	/*
	 * Now write the register info for the thread that caused the
	 * coredump.
	 */
	error = ELFNAMEEND(coredump_note)(p, l, iocookie, &notesize);
	if (error)
		return (error);
	size += notesize;

	/*
	 * Now, for each LWP, write the register info and any other
	 * per-LWP notes.
	 */
	LIST_FOREACH(l0, &p->p_lwps, l_sibling) {
		if (l0 == l)		/* we've taken care of this thread */
			continue;
		error = ELFNAMEEND(coredump_note)(p, l0, iocookie, &notesize);
		if (error)
			return (error);
		size += notesize;
	}

	*sizep = size;
	return (0);
}

static int
ELFNAMEEND(coredump_note)(struct proc *p, struct lwp *l, void *iocookie,
    size_t *sizep)
{
	Elf_Nhdr nhdr;
	int size, notesize, error;
	int namesize;
	char name[64+ELFROUNDSIZE];
	elf_reg intreg;
#ifdef PT_GETFPREGS
	elf_fpreg freg;
#endif

	size = 0;

	snprintf(name, sizeof(name)-ELFROUNDSIZE, "%s@%d",
	    ELF_NOTE_NETBSD_CORE_NAME, l->l_lid);
	namesize = strlen(name) + 1;
	memset(name + namesize, 0, elfround(namesize) - namesize);

	notesize = sizeof(nhdr) + elfround(namesize) + elfround(sizeof(intreg));
	if (iocookie) {
		PHOLD(l);
		error = elf_process_read_regs(l, &intreg);
		PRELE(l);
		if (error)
			return (error);

		nhdr.n_namesz = namesize;
		nhdr.n_descsz = sizeof(intreg);
		nhdr.n_type = PT_GETREGS;

		error = ELFNAMEEND(coredump_writenote)(p, iocookie, &nhdr,
		    name, &intreg);
		if (error)
			return (error);

	}
	size += notesize;

#ifdef PT_GETFPREGS
	notesize = sizeof(nhdr) + elfround(namesize) + elfround(sizeof(freg));
	if (iocookie) {
		PHOLD(l);
		error = elf_process_read_fpregs(l, &freg);
		PRELE(l);
		if (error)
			return (error);

		nhdr.n_namesz = namesize;
		nhdr.n_descsz = sizeof(freg);
		nhdr.n_type = PT_GETFPREGS;

		error = ELFNAMEEND(coredump_writenote)(p, iocookie, &nhdr,
		    name, &freg);
		if (error)
			return (error);
	}
	size += notesize;
#endif
	*sizep = size;
	/* XXX Add hook for machdep per-LWP notes. */
	return (0);
}

int
ELFNAMEEND(coredump_writenote)(struct proc *p, void *cookie, Elf_Nhdr *nhdr,
    const char *name, void *data)
{
	int error;

	error = coredump_write(cookie, UIO_SYSSPACE, nhdr, sizeof(*nhdr));
	if (error)
		return error;

	error = coredump_write(cookie, UIO_SYSSPACE, name,
	    elfround(nhdr->n_namesz));
	if (error)
		return error;

	return coredump_write(cookie, UIO_SYSSPACE, data, nhdr->n_descsz);
}
