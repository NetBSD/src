/* $NetBSD: kloader.c,v 1.2 2003/07/15 01:31:41 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: kloader.c,v 1.2 2003/07/15 01:31:41 lukem Exp $");

#include "opt_kloader.h"
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

#include <sh3/cache.h>
#include <sh3/cache_sh4.h>
#include <sh3/mmu_sh4.h>

#include <machine/kloader.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#ifdef KLOADER_DEBUG
int	kloader_debug = 1;
#define DPRINTF(fmt, args...)						\
	if (kloader_debug)						\
		printf("%s: " fmt, __FUNCTION__ , ##args) 
#define _DPRINTF(fmt, args...)						\
	if (kloader_debug)						\
		printf(fmt, ##args) 
#define DPRINTFN(n, arg)						\
	if (kloader_debug > (n))					\
		printf("%s: " fmt, __FUNCTION__ , ##args) 
#else
#define DPRINTF(arg...)		((void)0)
#define _DPRINTF(arg...)	((void)0)
#define DPRINTFN(n, arg...)	((void)0)
#endif

struct kloader_page_tag {
	u_int32_t next;
	u_int32_t src;
	u_int32_t dst;
	u_int32_t sz;
};
#define BUCKET_SIZE	(PAGE_SIZE - sizeof(struct kloader_page_tag))

STATIC struct {
	struct pglist pg_head;
#define PG_VADDR(pg)	SH3_PHYS_TO_P1SEG(VM_PAGE_TO_PHYS(pg))
	struct vm_page *cur_pg;
	struct vnode *vp;
	vaddr_t entry;
	struct kloader_page_tag *tagstart;
	void (*loader)(vaddr_t, struct kloader_page_tag *);
	int setuped;
} kloader;

extern paddr_t avail_start, avail_end;
STATIC void kloader_boot(u_int32_t, struct kloader_page_tag *);
STATIC int kloader_load(void);
STATIC int kloader_alloc_memory(size_t);
STATIC void kloader_load_segment(vaddr_t, vsize_t, off_t, size_t);
STATIC void kloader_load_segment_end(void);
STATIC void kloader_load_bucket(vaddr_t, off_t, size_t);

STATIC struct vnode *kloader_open(const char *);
STATIC void kloader_close(void);
STATIC int kloader_read(size_t, size_t, void *);

#ifdef KLOADER_DEBUG
STATIC void kloader_dump(vaddr_t, struct kloader_page_tag *);
#endif

void
kloader_reboot_setup(const char *filename)
{

	DPRINTF("boot file name: %s\n", filename);
	kloader.vp = kloader_open(filename);
	if (kloader.vp == NULL)
		return;

	if (kloader_load() != 0)
		goto end;

	kloader.setuped = 1;
#ifdef KLOADER_DEBUG
	kloader_dump(kloader.entry, kloader.tagstart);
#endif
 end:
	kloader_close();
}

void
kloader_reboot()
{

	if (!kloader.setuped)
		return;

	sh_icache_sync_all();
	(*kloader.loader)(kloader.entry, kloader.tagstart);
	/* NOTREACHED */
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
		DPRINTF("not a ELF file\n");
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

	memcpy(kloader.loader, kloader_boot, PAGE_SIZE);

	kloader.entry = eh.e_entry;

	return (0);
}

int
kloader_alloc_memory(size_t sz)
{
	int n, error;

	n = (sz + BUCKET_SIZE - 1) / BUCKET_SIZE + 1; /* 2nd loader */

	error = uvm_pglistalloc(n * PAGE_SIZE, avail_start, avail_end,
	    PAGE_SIZE, 0, &kloader.pg_head, n, 0);
	if (error) {
		DPRINTF("can't allocate memory.\n");
		return (1);
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
	struct proc *p = curproc;
	struct nameidata nid;

	NDINIT(&nid, LOOKUP, FOLLOW, UIO_SYSSPACE, filename, p);

	if (namei(&nid) != 0) {
		DPRINTF("namei failed (%s)\n", filename);
		return (0);
	}

	if (vn_open(&nid, FREAD, 0) != 0) {
		DPRINTF("open failed\n");
		return (0);
	}

	return (nid.ni_vp);
}

void
kloader_close()
{
	struct proc *p = curproc;
	struct vnode *vp = kloader.vp;

	VOP_UNLOCK(vp, 0);
	vn_close(vp, FREAD, p->p_ucred, p);
}

int
kloader_read(size_t ofs, size_t size, void *buf)
{
	struct proc *p = curproc;
	struct vnode *vp = kloader.vp;
	size_t resid;
	int error;

	error = vn_rdwr(UIO_READ, vp, buf, size, ofs, UIO_SYSSPACE,
	    IO_NODELOCKED | IO_SYNC, p->p_ucred, &resid, p);

	if (error)
		DPRINTF("read error.\n");

	return (error);
}

void
kloader_boot(u_int32_t entry, struct kloader_page_tag *p)
{
	int tmp;

	/* Disable interrupt. block exception. */
	__asm__ __volatile__(
		"stc	sr, %1;"
		"or	%0, %1;"
		"ldc	%1, sr" : : "r"(0x500000f0), "r"(tmp));

	/* Now I run on P1, TLB flush. and disable. */
	SH4_TLB_DISABLE;

	do {
		u_int32_t *dst =(u_int32_t *)p->dst;
		u_int32_t *src =(u_int32_t *)p->src;
		u_int32_t sz = p->sz / sizeof (int);
		while (sz--)
			*dst++ = *src++;
	} while ((p = (struct kloader_page_tag *)p->next) != 0);

	SH7750_CACHE_FLUSH();

	/* jump to kernel entry. */
	__asm__ __volatile__(
		"jmp	@%0;"
		"nop;"
		: : "r"(entry));
	/* NOTREACHED */
}

#ifdef KLOADER_DEBUG
void
kloader_dump(vaddr_t entry, struct kloader_page_tag *tag)
{
	struct kloader_page_tag *p, *op;
	int i, n;

	p = tag;
	i = 0, n = 30;
	do  {
		if (i < n) {
			DPRINTF("[%d] next 0x%08x src 0x%08x dst 0x%08x"
			    " sz 0x%x\n", i, p->next, p->src, p->dst, p->sz);
		} else if (i == n) {
			DPRINTF("[...]\n");
		}
		op = p;
		i++;
	} while ((p = (struct kloader_page_tag *)(p->next)) != 0);
  
	DPRINTF("[%d(last)] next 0x%08x src 0x%08x dst 0x%08x sz 0x%x\n",
	    i - 1, op->next, op->src, op->dst, op->sz);
}
#endif /* KLOADER_DEBUG */

