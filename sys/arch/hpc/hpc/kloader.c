/*	$NetBSD: kloader.c,v 1.3.2.3 2002/04/01 07:40:02 nathanw Exp $	*/

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

#include "debug_kloader.h"

#include <sys/param.h>
#include <sys/systm.h>
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
#endif
#define USE_HPC_DPRINTF
#define __DPRINTF_EXT
#include <machine/debug.h>

struct kloader {
	struct pglist pg_head;
	struct vm_page *cur_pg;
	struct vnode *vp;
	struct kloader_page_tag *tagstart;
	struct kloader_bootinfo *bootinfo;
	vaddr_t loader_sp;
	kloader_bootfunc_t *loader;
	int setuped;
	int called;

	struct kloader_ops *ops;
};

#define BUCKET_SIZE	(PAGE_SIZE - sizeof(struct kloader_page_tag))
#define KLOADER_PROC	(&proc0)
STATIC struct kloader kloader;

STATIC int kloader_load(void);
STATIC int kloader_alloc_memory(size_t);
STATIC void kloader_load_segment(vaddr_t, vsize_t, off_t, size_t);
STATIC void kloader_load_segment_end(void);
STATIC void kloader_load_bucket(vaddr_t, off_t, size_t);
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
	    kloader.bootinfo, kloader.tagstart);
}

int
kloader_load()
{
	Elf_Ehdr eh;
	Elf_Phdr ph[16], *p;
	Elf_Shdr sh[16];
	vaddr_t kv;
	size_t sz;
	int i;

	/* read kernel's ELF header */
	kloader_read(0, sizeof(Elf_Ehdr), &eh);
  
	if (eh.e_ident[EI_MAG0] != ELFMAG0 ||
	    eh.e_ident[EI_MAG1] != ELFMAG1 ||
	    eh.e_ident[EI_MAG2] != ELFMAG2 ||
	    eh.e_ident[EI_MAG3] != ELFMAG3) {
		PRINTF("not a ELF file\n");
		return (1);
	}

	/* read section header */
	kloader_read(eh.e_shoff, eh.e_shentsize * eh.e_shnum, sh);

	/* read program header */
	kloader_read(eh.e_phoff, eh.e_phentsize * eh.e_phnum, ph);

	/* calcurate memory size */
	DPRINTF("file size: ");
	for (sz = 0, i = 0; i < eh.e_phnum; i++) {
		if (ph[i].p_type == PT_LOAD) {
			size_t filesz = ph[i].p_filesz;
			_DPRINTF("+0x%x", filesz);
			sz += round_page(filesz);
		}
	}
	_DPRINTF(" entry: %08x\n", eh.e_entry);

	/* get memory for new kernel */
	if (kloader_alloc_memory(sz) != 0)
		return (1);

	/* copy newkernel to memory */
	for (i = 0, p = ph; i < eh.e_phnum; i++, p++) {
		if (p->p_type == PT_LOAD) {
			size_t filesz = p->p_filesz;
			size_t memsz = p->p_memsz;
			off_t fileofs = p->p_offset;
			kv = p->p_vaddr;
			DPRINTF("[%d] vaddr 0x%08lx file size 0x%x"
				" mem size 0x%x\n", i, kv, filesz, memsz);
			kloader_load_segment(kv, memsz, fileofs, filesz);
			kv += memsz;
		}
	}
	/* end tag */
	kloader_load_segment_end();

	/* copy loader code */
	KDASSERT(kloader.cur_pg);
	kloader.loader = (void *)PG_VADDR(kloader.cur_pg);
	memcpy(kloader.loader, kloader.ops->boot, PAGE_SIZE);

	/* loader stack */
	kloader.loader_sp = (vaddr_t)kloader.loader + PAGE_SIZE;

	/* kernel entry point */
	kloader.bootinfo->entry = eh.e_entry;

	DPRINTF("[loader] addr=%p sp=0x%08lx [kernel] addr=0x%08lx\n",
	    kloader.loader, kloader.loader_sp, kloader.bootinfo->entry);

	return (0);
}

int
kloader_alloc_memory(size_t sz)
{
	extern paddr_t avail_start, avail_end;
	int i, n, error;

	n = (sz + BUCKET_SIZE - 1) / BUCKET_SIZE	/* kernel */
	    + 1;					/* 2nd loader */

	TAILQ_INIT(&kloader.pg_head);

	for (i = 0; i < n; i++) {
		error = uvm_pglistalloc(PAGE_SIZE, avail_start, avail_end,
		    PAGE_SIZE, PAGE_SIZE, &kloader.pg_head, 0, 0);
		if (error) {
			uvm_pglistfree(&kloader.pg_head);
			PRINTF("can't allocate memory.\n");
			return (1);
		}
	}

	kloader.cur_pg = TAILQ_FIRST(&kloader.pg_head);
	kloader.tagstart = (void *)PG_VADDR(kloader.cur_pg);

	return (0);
}

void kloader_load_segment(vaddr_t kv, vsize_t memsz, off_t fileofs,
    size_t filesz)
{
	int i, n;

	if (filesz == 0)
		return;

	n = filesz / BUCKET_SIZE;
	for (i = 0; i < n; i ++) {
		kloader_load_bucket(kv, fileofs, BUCKET_SIZE);
		kv += BUCKET_SIZE;
		fileofs += BUCKET_SIZE;
	}

	n = filesz % BUCKET_SIZE;
	if (n) {
		kloader_load_bucket(kv, fileofs, n);
	}
}

void
kloader_load_segment_end()
{
	struct vm_page *pg;

	pg = TAILQ_PREV(kloader.cur_pg, pglist, pageq);
	
	((struct kloader_page_tag *)PG_VADDR(pg))->next = 0;
}

void
kloader_load_bucket(vaddr_t kv, off_t ofs, size_t sz)
{
	struct vm_page *pg = kloader.cur_pg;
	vaddr_t addr = PG_VADDR(pg);
	struct kloader_page_tag *tag = (void *)addr;

	KDASSERT(pg != NULL);

	tag->src = addr + sizeof(struct kloader_page_tag);
	tag->dst = kv;
	tag->sz = sz;

	kloader_read(ofs, sz, (void *)tag->src);

	pg = TAILQ_NEXT(pg, pageq);
	tag->next = PG_VADDR(pg);
	kloader.cur_pg = pg;
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
	PRINT(timezone, 0x%08lx);
#undef PRINT

	printf("[args]\n");
	for (i = 0; i < kbi->argc; i++) {
		printf(" - %s\n", kbi->argv[i]);
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
