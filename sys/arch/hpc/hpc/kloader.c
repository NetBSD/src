/*	$NetBSD: kloader.c,v 1.11 2004/03/27 02:01:07 uwe Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kloader.c,v 1.11 2004/03/27 02:01:07 uwe Exp $");

#include "debug_kloader.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/fcntl.h>
#define ELFSIZE	32
#include <sys/exec_elf.h>

#include <uvm/uvm_extern.h>

#include <machine/kloader.h>

#ifdef KLOADER_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	kloader_debug
#define DPRINTF_LEVEL	1
#endif
#define USE_HPC_DPRINTF
#define __DPRINTF_EXT
#include <machine/debug.h>

struct kloader {
	struct pglist pg_head;
	struct vm_page *cur_pg;
	struct kloader_page_tag *cur_tag;
	struct vnode *vp;
	struct kloader_page_tag *tagstart;
	struct kloader_bootinfo *bootinfo;
	struct kloader_bootinfo *rebootinfo;
	vaddr_t loader_sp;
	kloader_bootfunc_t *loader;
	int setuped;
	int called;

	struct kloader_ops *ops;
};

#define BUCKET_SIZE	(PAGE_SIZE - sizeof(struct kloader_page_tag))
#define KLOADER_PROC	(&proc0)
STATIC struct kloader kloader;

#define ROUND4(x)	(((x) + 3) & ~3)


STATIC int kloader_load(void);

STATIC int kloader_alloc_memory(size_t);
STATIC struct kloader_page_tag *kloader_get_tag(vaddr_t);
STATIC void kloader_from_file(vaddr_t, off_t, size_t);
STATIC void kloader_copy(vaddr_t, const void *, size_t);
STATIC void kloader_zero(vaddr_t, size_t);

STATIC void kloader_load_segment(Elf_Phdr *);

STATIC struct vnode *kloader_open(const char *);
STATIC void kloader_close(void);
STATIC int kloader_read(size_t, size_t, void *);

#ifdef KLOADER_DEBUG
STATIC void kloader_pagetag_dump(void);
STATIC void kloader_bootinfo_dump(void);
#endif


void
__kloader_reboot_setup(struct kloader_ops *ops, const char *filename)
{
	static const char fatal_msg[] = "*** FATAL ERROR ***\n";

	if (kloader.bootinfo == NULL) {
		PRINTF("No bootinfo.\n");
		return;
	}

	if (ops == NULL || ops->jump == NULL || ops->boot == NULL) {
		PRINTF("No boot operations.\n");
		return;
	}
	kloader.ops = ops;

	switch (kloader.called++) {
	case 0:
		/* normal operation */
		break;
 	case 1:
		/* try to reboot anyway. */
		printf("%s", fatal_msg);
		kloader.setuped = TRUE;
		kloader_load ();
		kloader_reboot();
		/* NOTREACHED */
		break;
	case 2:
		/* if reboot method exits, reboot. */
		printf("%s", fatal_msg);
		/* try to reset */
		if (kloader.ops->reset != NULL)
			(*kloader.ops->reset) ();
		/* NOTREACHED */
		break;
	}

	PRINTF("kernel file name: %s\n", filename);
	kloader.vp = kloader_open(filename);
	if (kloader.vp == NULL)
		return;

	if (kloader_load() != 0)
		goto end;

	kloader.setuped = TRUE;
#ifdef KLOADER_DEBUG
	kloader_pagetag_dump();
#endif
 end:
	kloader_close();
}


void
kloader_reboot()
{

	if (!kloader.setuped)
		return;

	printf("Rebooting...\n");

	(*kloader.ops->jump) (kloader.loader, kloader.loader_sp,
	    kloader.rebootinfo, kloader.tagstart);
}


int
kloader_load()
{
	Elf_Ehdr eh;
	Elf_Phdr ph[16], *p;
	Elf_Shdr sh[16];
	Elf_Addr entry;
	vaddr_t kv;
	size_t sz;
	size_t shstrsz;
	char *shstrtab;
	int symndx, strndx;
	size_t ksymsz;
	struct kloader_bootinfo nbi; /* new boot info */
	char *oldbuf, *newbuf;
	char **ap;
	int i;

	/* read kernel's ELF header */
	kloader_read(0, sizeof(Elf_Ehdr), &eh);
  
	if (eh.e_ident[EI_MAG0] != ELFMAG0 ||
	    eh.e_ident[EI_MAG1] != ELFMAG1 ||
	    eh.e_ident[EI_MAG2] != ELFMAG2 ||
	    eh.e_ident[EI_MAG3] != ELFMAG3) {
		PRINTF("not an ELF file\n");
		return (1);
	}

	/* read program headers */
	kloader_read(eh.e_phoff, eh.e_phentsize * eh.e_phnum, ph);

	/* read section headers */
	kloader_read(eh.e_shoff, eh.e_shentsize * eh.e_shnum, sh);

	/* read section names */
	shstrsz = ROUND4(sh[eh.e_shstrndx].sh_size);
	shstrtab = malloc(shstrsz, M_TEMP, M_NOWAIT);
	if (shstrtab == NULL) {
		PRINTF("unable to allocate memory for .shstrtab\n");
		return (1);
	}
	DPRINTF("reading 0x%x bytes of .shstrtab at 0x%x\n",
		sh[eh.e_shstrndx].sh_size, sh[eh.e_shstrndx].sh_offset);
	kloader_read(sh[eh.e_shstrndx].sh_offset, sh[eh.e_shstrndx].sh_size,
		     shstrtab);

	/* save entry point, code to construct symbol table overwrites it */
	entry = eh.e_entry;


	/*
	 * Calcurate memory size
	 */
	sz = 0;

	/* loadable segments */
	for (i = 0; i < eh.e_phnum; i++) {
		if (ph[i].p_type == PT_LOAD) {
			DPRINTF("segment %d size = file 0x%x memory 0x%x\n",
				i, ph[i].p_filesz, ph[i].p_memsz);
#ifdef KLOADER_ZERO_BSS
			sz += round_page(ph[i].p_memsz);
#else
			sz += round_page(ph[i].p_filesz);
#endif
			sz += PAGE_SIZE; /* compensate for partial last tag */
		}
	}

	if (sz == 0)		/* nothing to load? */
		return (1);

	/* symbols/strings sections */
	symndx = strndx = -1;
	for (i = 0; i < eh.e_shnum; i++) {
	    if (strcmp(shstrtab + sh[i].sh_name, ".symtab") == 0)
		    symndx = i;
	    else if (strcmp(shstrtab + sh[i].sh_name, ".strtab") == 0)
		    strndx = i;
	    else if (i != eh.e_shstrndx)
		    /* while here, mark all other sections as unused */
		    sh[i].sh_type = SHT_NULL;
	}

	if (symndx < 0 || strndx < 0) {
		if (symndx < 0)
			PRINTF("no .symtab section\n");
		if (strndx < 0)
			PRINTF("no .strtab section\n");
		ksymsz = SELFMAG; /* just a bad magic */
	} else {
		ksymsz = sizeof(Elf_Ehdr)
			+ eh.e_shentsize * eh.e_shnum
			+ shstrsz		/* rounded to 4 bytes */
			+ sh[symndx].sh_size
			+ sh[strndx].sh_size;
		DPRINTF("ksyms size = 0x%x\n", ksymsz);
	}
	sz += ROUND4(ksymsz);

	/* boot info for the new kernel */
	sz += sizeof(struct kloader_bootinfo);

	/* get memory for new kernel */
	if (kloader_alloc_memory(sz) != 0)
		return (1);


	/*
	 * Copy new kernel in.
	 */
	kv = 0;			/* XXX: -Wuninitialized */
	for (i = 0, p = ph; i < eh.e_phnum; i++, p++) {
		if (p->p_type == PT_LOAD) {
			kloader_load_segment(p);
			kv = p->p_vaddr + p->p_memsz;
		}
	}


	/*
	 * Construct symbol table for ksyms.
	 */
	if (symndx < 0 || strndx < 0) {
		kloader_zero(kv, SELFMAG);
		kv += SELFMAG;
	} else {
		Elf_Off eoff;
		off_t symoff, stroff;

		/* save offsets of .symtab and .strtab before we change them */
		symoff = sh[symndx].sh_offset;
		stroff = sh[strndx].sh_offset;

		/* no loadable segments */
		eh.e_entry = 0;
		eh.e_phnum = 0;
		eh.e_phoff = 0;

		/* change offsets to reflect new layout */
		eoff = sizeof(Elf_Ehdr);
		eh.e_shoff = eoff;

		eoff += eh.e_shentsize * eh.e_shnum;
		sh[eh.e_shstrndx].sh_offset = eoff;

		eoff += shstrsz;
		sh[symndx].sh_offset = eoff;

		eoff += sh[symndx].sh_size;
		sh[strndx].sh_offset = eoff;

		/* local copies massaged, can serve them now */
		DPRINTF("ksyms ELF header\n");
		kloader_copy(kv, &eh, sizeof(Elf_Ehdr));
		kv += sizeof(Elf_Ehdr);

		DPRINTF("ksyms section headers\n");
		kloader_copy(kv, sh, eh.e_shentsize * eh.e_shnum);
		kv += eh.e_shentsize * eh.e_shnum;

		DPRINTF("ksyms .shstrtab\n");
		kloader_copy(kv, shstrtab, shstrsz);
		kv += shstrsz;

		DPRINTF("ksyms .symtab\n");
		kloader_from_file(kv, symoff, sh[symndx].sh_size);
		kv += sh[symndx].sh_size;

		DPRINTF("ksyms .strtab\n");
		kloader_from_file(kv, stroff, ROUND4(sh[strndx].sh_size));
		kv += ROUND4(sh[strndx].sh_size);
	}

	/*
	 * Create boot info to pass to the new kernel.
	 * All pointers in it are *not* valid until the new kernel runs!
	 */

	/* get a private copy of current bootinfo to vivisect */
	memcpy(&nbi, kloader.bootinfo,
	       sizeof(struct kloader_bootinfo));

	/* new kernel entry point */
	nbi.entry = entry;

	/* where args currently are, see kloader_bootinfo_set() */
	oldbuf = &kloader.bootinfo->_argbuf[0];

	/* where args *will* be after boot code copied them */
	newbuf = (char *)(void *)kv
		+ offsetof(struct kloader_bootinfo, _argbuf);

	DPRINTF("argv: old %p -> new %p\n", oldbuf, newbuf);

	/* not a valid pointer in this kernel! */
	nbi.argv = (void *)newbuf;

	/* local copy that we populate with new (not yet valid) pointers */
	ap = (char **)(void *)nbi._argbuf;

	for (i = 0; i < kloader.bootinfo->argc; ++i) {
		DPRINTFN(1, " [%d]: %p -> ", i, kloader.bootinfo->argv[i]);
		ap[i] = newbuf +
			(kloader.bootinfo->argv[i] - oldbuf);
		_DPRINTFN(1, "%p\n", ap[i]);
	}

	/* arrange for the new bootinfo to get copied */
	DPRINTF("bootinfo\n");
	kloader_copy(kv, &nbi, sizeof(struct kloader_bootinfo));

	/* will be valid by the time the new kernel starts */
	kloader.rebootinfo = (void *)kv;
	/* kv += sizeof(struct kloader_bootinfo); */


	/*
	 * Copy loader code
	 */
	KDASSERT(kloader.cur_pg);
	kloader.loader = (void *)PG_VADDR(kloader.cur_pg);
	memcpy(kloader.loader, kloader.ops->boot, PAGE_SIZE);

	/* loader stack starts at the bottom of that page */
	kloader.loader_sp = (vaddr_t)kloader.loader + PAGE_SIZE;

	DPRINTF("[loader] addr=%p sp=%p [kernel] entry=%p\n",
		kloader.loader, (void *)kloader.loader_sp, (void *)nbi.entry);

	return (0);
}


int
kloader_alloc_memory(size_t sz)
{
	extern paddr_t avail_start, avail_end;
	int n, error;

	n = (sz + BUCKET_SIZE - 1) / BUCKET_SIZE	/* kernel &co */
	    + 1;					/* 2nd loader */

	error = uvm_pglistalloc(n * PAGE_SIZE, avail_start, avail_end,
				PAGE_SIZE, 0, &kloader.pg_head, n, 0);
	if (error) {
		PRINTF("can't allocate memory.\n");
		return (1);
	}

	kloader.cur_pg = TAILQ_FIRST(&kloader.pg_head);
	kloader.tagstart = (void *)PG_VADDR(kloader.cur_pg);
	kloader.cur_tag = NULL;

	return (0);
}


struct kloader_page_tag *
kloader_get_tag(vaddr_t dst)
{
	struct vm_page *pg;
	vaddr_t addr;
	struct kloader_page_tag *tag;

	tag = kloader.cur_tag;
	if (tag != NULL		/* has tag */
	    && tag->sz < BUCKET_SIZE /* that has free space */
	    && tag->dst + tag->sz == dst) /* and new data are contiguous */
	{
		DPRINTFN(1, "current tag %x/%x ok\n", tag->dst, tag->sz);
		return (tag);
	}

	pg = kloader.cur_pg;
	KDASSERT(pg != NULL);
	kloader.cur_pg = TAILQ_NEXT(pg, pageq);

	addr = PG_VADDR(pg);
	tag = (void *)addr;

	tag->src = addr + sizeof(struct kloader_page_tag);
	tag->dst = dst;
	tag->sz = 0;
	if (kloader.cur_tag)
		kloader.cur_tag->next = addr;
	kloader.cur_tag = tag;

	return (tag);
}


/*
 * Operations to populate kloader_page_tag's with data.
 */

void
kloader_from_file(vaddr_t dst, off_t ofs, size_t sz)
{
	struct kloader_page_tag *tag;
	size_t freesz;

	while (sz > 0) {
		tag = kloader_get_tag(dst);
		KDASSERT(tag != NULL);
		freesz = BUCKET_SIZE - tag->sz;
		if (freesz > sz)
			freesz = sz;

		DPRINTFN(1, "0x%08lx + 0x%x <- 0x%lx\n", dst, freesz,
			 (unsigned long)ofs);
		kloader_read(ofs, freesz, (void *)(tag->src + tag->sz));

		tag->sz += freesz;
		sz -= freesz;
		ofs += freesz;
		dst += freesz;
	}
}


void
kloader_copy(vaddr_t dst, const void *src, size_t sz)
{
	struct kloader_page_tag *tag;
	size_t freesz;

	while (sz > 0) {
		tag = kloader_get_tag(dst);
		KDASSERT(tag != NULL);
		freesz = BUCKET_SIZE - tag->sz;
		if (freesz > sz)
			freesz = sz;

		DPRINTFN(1, "0x%08lx + 0x%x <- %p\n", dst, freesz, src);
		memcpy((void *)(tag->src + tag->sz), src, freesz);

		tag->sz += freesz;
		sz -= freesz;
		src = (char *)src + freesz;
		dst += freesz;
	}
}


void
kloader_zero(vaddr_t dst, size_t sz)
{
	struct kloader_page_tag *tag;
	size_t freesz;

	while (sz > 0) {
		tag = kloader_get_tag(dst);
		KDASSERT(tag != NULL);
		freesz = BUCKET_SIZE - tag->sz;
		if (freesz > sz)
			freesz = sz;

		DPRINTFN(1, "0x%08lx + 0x%x\n", dst, freesz);
		memset((void *)(tag->src + tag->sz), 0, freesz);

		tag->sz += freesz;
		sz -= freesz;
		dst += freesz;
	}
}


void
kloader_load_segment(Elf_Phdr *p)
{

	DPRINTF("memory 0x%08x 0x%x <- file 0x%x 0x%x\n",
		p->p_vaddr, p->p_memsz, p->p_offset, p->p_filesz);

	kloader_from_file(p->p_vaddr, p->p_offset, p->p_filesz);
#ifdef KLOADER_ZERO_BSS
	kloader_zero(p->p_vaddr + p->p_filesz, p->p_memsz - p->p_filesz);
#endif
}


/*
 * file access
 */
struct vnode *
kloader_open(const char *filename)
{
	struct proc *p = KLOADER_PROC;
	struct nameidata nid;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, filename, p);

	if (namei(&nid) != 0) {
		PRINTF("namei failed (%s)\n", filename);
		return (0);
	}

	if (vn_open(&nid, FREAD, 0) != 0) {
		PRINTF("%s open failed\n", filename);
		return (0);
	}

	return (nid.ni_vp);
}

void
kloader_close()
{
	struct proc *p = KLOADER_PROC;
	struct vnode *vp = kloader.vp;

	VOP_UNLOCK(vp, 0);
	vn_close(vp, FREAD, p->p_ucred, p);
}

int
kloader_read(size_t ofs, size_t size, void *buf)
{
	struct proc *p = KLOADER_PROC;
	struct vnode *vp = kloader.vp;
	size_t resid;
	int error;

	error = vn_rdwr(UIO_READ, vp, buf, size, ofs, UIO_SYSSPACE,
	    IO_NODELOCKED | IO_SYNC, p->p_ucred, &resid, p);

	if (error)
		PRINTF("read error.\n");

	return (error);
}


/*
 * bootinfo
 */
void
kloader_bootinfo_set(struct kloader_bootinfo *kbi, int argc, char *argv[],
    struct bootinfo *bi, int printok)
{
	char *p, *pend, *buf;
	int i;

	kloader.bootinfo = kbi;
	buf = kbi->_argbuf;

	memcpy(&kbi->bootinfo, bi, sizeof(struct bootinfo));
	kbi->argc = argc;
	kbi->argv = (char **)buf;

	p = &buf[argc * sizeof(char **)];
	pend = &buf[KLOADER_KERNELARGS_MAX - 1];

	for (i = 0; i < argc; i++) {
		char *q = argv[i];
		int len = strlen(q) + 1;
		if ((p + len) > pend) {
			kloader.bootinfo = NULL;
			if (printok)
				PRINTF("buffer insufficient.\n");
			return;
		}
		kbi->argv[i] = p;
		memcpy(p, q, len);
		p += len;
	}
#ifdef KLOADER_DEBUG
	if (printok)
		kloader_bootinfo_dump();
#endif
}


/*
 * debug
 */
#ifdef KLOADER_DEBUG
void
kloader_bootinfo_dump()
{
	struct kloader_bootinfo *kbi = kloader.bootinfo;
	struct bootinfo *bi;
	int i;

	if (kbi == NULL) {
		PRINTF("no bootinfo available.\n");
		return;
	}
	bi = &kbi->bootinfo;

	dbg_banner_function();
	printf("[bootinfo] addr=%p\n", kbi);
#define PRINT(m, fmt)	printf(" - %-15s= " #fmt "\n", #m, bi->m)
	PRINT(length, %d);
	PRINT(magic, 0x%08x);
	PRINT(fb_addr, %p);
	PRINT(fb_line_bytes, %d);
	PRINT(fb_width, %d);
	PRINT(fb_height, %d);
	PRINT(fb_type, %d);
	PRINT(bi_cnuse, %d);
	PRINT(platid_cpu, 0x%08lx);
	PRINT(platid_machine, 0x%08lx);
	PRINT(timezone, %ld);
#undef PRINT

	printf("[args: %d at %p]\n", kbi->argc, kbi->argv);
	for (i = 0; i < kbi->argc; i++) {
		printf("[%d] at %p = \"%s\"\n", i, kbi->argv[i], kbi->argv[i]);
	}
	dbg_banner_line();
}

void
kloader_pagetag_dump()
{
	struct kloader_page_tag *tag = kloader.tagstart;
	struct kloader_page_tag *p, *op;
	int i, n;

	p = tag;
	i = 0, n = 15;

	dbg_banner_function();
	PRINTF("[page tag chain]\n");
	do  {
		if (i < n) {
			printf("[%2d] next 0x%08x src 0x%08x dst 0x%08x"
			    " sz 0x%x\n", i, p->next, p->src, p->dst, p->sz);
		} else if (i == n) {
			printf("[...]\n");
		}
		op = p;
		i++;
	} while ((p = (struct kloader_page_tag *)(p->next)) != 0);
  
	printf("[%d(last)] next 0x%08x src 0x%08x dst 0x%08x sz 0x%x\n",
	    i - 1, op->next, op->src, op->dst, op->sz);
	dbg_banner_line();
}
#endif /* KLOADER_DEBUG */
