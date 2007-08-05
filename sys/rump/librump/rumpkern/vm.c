/*	$NetBSD: vm.c,v 1.1 2007/08/05 22:28:10 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Virtual memory emulation routines.  Contents:
 *  + UBC
 *  + anon objects & pager
 *  + vnode objects & pager
 *  + misc support routines
 */

/*
 * XXX: we abuse pg->uobject for the virtual address of the storage
 * for each page.  phys_addr would fit the job description better,
 * except that it will create unnecessary lossage on some platforms
 * due to not being a pointer type.
 */

#include <sys/param.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/buf.h>

#include <uvm/uvm.h>
#include <uvm/uvm_prot.h>

#include <machine/pmap.h>

#include "rump.h"
#include "rumpuser.h"

/* dumdidumdum */
#define len2npages(off, len)						\
  (((((len) + PAGE_MASK) & ~(PAGE_MASK)) >> PAGE_SHIFT)			\
    + (((off & PAGE_MASK) + (len & PAGE_MASK)) > PAGE_SIZE))

static int ubc_winvalid;
static struct uvm_object *ubc_uobj;
static off_t ubc_offset;
static int ubc_flags;

struct uvm_pagerops uvm_vnodeops;
struct uvm_pagerops aobj_pager;
struct uvmexp uvmexp;

struct vmspace rump_vmspace;
struct vm_map rump_vmmap;

/*
 * vm pages 
 */

/* XXX: we could be smarter about this */
struct vm_page *
rumpvm_findpage(struct uvm_object *uobj, voff_t off)
{
	struct vm_page *pg;

	TAILQ_FOREACH(pg, &uobj->memq, listq)
		if (pg->offset == off)
			return pg;

	return NULL;
}

struct vm_page *
rumpvm_makepage(struct uvm_object *uobj, voff_t off, int allocstorage)
{
	struct vm_page *pg;

	pg = rumpuser_malloc(sizeof(struct vm_page), 0);
	memset(pg, 0, sizeof(struct vm_page));
	TAILQ_INSERT_TAIL(&uobj->memq, pg, listq);
	pg->offset = off;

	if (allocstorage) {
		pg->uobject = (void *)rumpuser_malloc(PAGE_SIZE, 0);
		memset((void *)pg->uobject, 0, PAGE_SIZE);
	}

	return pg;
}

void
rumpvm_freepage(struct uvm_object *uobj, struct vm_page *pg)
{

	TAILQ_REMOVE(&uobj->memq, pg, listq);
	rumpuser_free((void *)pg->uobject);
	rumpuser_free(pg);
}

/*
 * vnode pager
 */

int
rump_vopwrite_fault(struct vnode *vp, voff_t offset, size_t len,
	kauth_cred_t cred)
{
	int npages = len2npages(offset, len);
	struct vm_page *pgs[npages];
	int rv;

	if (offset >= vp->v_size)
		return 0;

	rv = VOP_GETPAGES(vp, offset, pgs, &npages, 0, 0, 0, 0);
	if (rv)
		return rv;

	assert(npages == len2npages(offset, len));

	return 0;
}

static int
vn_get(struct uvm_object *uobj, voff_t off, struct vm_page **pgs,
	int *npages, int centeridx, vm_prot_t access_type,
	int advice, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;

	return VOP_GETPAGES(vp, off, pgs, npages, centeridx, access_type,
	    advice, flags);
}

static int
vn_put(struct uvm_object *uobj, voff_t offlo, voff_t offhi, int flags)
{
	struct vnode *vp = (struct vnode *)uobj;

	return VOP_PUTPAGES(vp, offlo, offhi, flags);
}

/*
 * Anon object stuff
 */

static int
ao_get(struct uvm_object *uobj, voff_t off, struct vm_page **pgs,
	int *npages, int centeridx, vm_prot_t access_type,
	int advice, int flags)
{
	struct vm_page *pg;
	int i;

	if (centeridx)
		panic("%s: centeridx != 0 not supported", __func__);

	/* loop over pages */
	off = trunc_page(off);
	for (i = 0; i < *npages; i++) {
		pg = rumpvm_findpage(uobj, off + (i << PAGE_SHIFT));
		if (pg) {
			pgs[i] = pg;
		} else {
			pg = rumpvm_makepage(uobj, off + (i << PAGE_SHIFT), 1);
			pgs[i] = pg;
		}
	}

	return 0;

}

static int
ao_put(struct uvm_object *uobj, voff_t start, voff_t stop, int flags)
{
	struct vm_page *pg;

	/* we only free all pages for now */
	if ((flags & PGO_FREE) == 0 || (flags & PGO_ALLPAGES) == 0)
		return 0;

	while ((pg = TAILQ_FIRST(&uobj->memq)) != NULL)
		rumpvm_freepage(uobj, pg);

	return 0;
}

struct uvm_object *
uao_create(vsize_t size, int flags)
{
	struct uvm_object *uobj;

	uobj = rumpuser_malloc(sizeof(struct uvm_object), 0);
	memset(uobj, 0, sizeof(struct uvm_object));
	uobj->pgops = &aobj_pager;
	TAILQ_INIT(&uobj->memq);

	return uobj;
}

void
uao_detach(struct uvm_object *uobj)
{

	ao_put(uobj, 0, 0, PGO_ALLPAGES | PGO_FREE);
	rumpuser_free(uobj);
}

/*
 * UBC
 */

int
rump_ubc_magic_uiomove(size_t n, struct uio *uio)
{
	int npages = len2npages(uio->uio_offset, n);
	struct vm_page *pgs[npages];
	int i, rv;

	if (ubc_winvalid == 0)
		panic("%s: ubc window not allocated", __func__);

	rv = ubc_uobj->pgops->pgo_get(ubc_uobj, ubc_offset & ~PAGE_MASK,
	    pgs, &npages, 0, 0, 0, 0);
	if (rv)
		return rv;

	for (i = 0; i < npages; i++) {
		size_t xfersize;
		off_t pageoff;

		pageoff = uio->uio_offset & PAGE_MASK;
		xfersize = MIN(MIN(n, PAGE_SIZE), PAGE_SIZE-pageoff);
		uiomove((uint8_t *)pgs[i]->uobject + pageoff, xfersize, uio);
		if (uio->uio_rw == UIO_WRITE)
			pgs[i]->flags &= ~PG_CLEAN;
		ubc_offset += xfersize;
		n -= xfersize;
	}

	return 0;
}

void *
ubc_alloc(struct uvm_object *uobj, voff_t offset, vsize_t *lenp, int advice,
	int flags)
{
	vsize_t reallen;

	/* XXX: only one window, but that's ok for now */
	if (ubc_winvalid == 1)
		panic("%s: ubc window already allocated", __func__);

	printf("UBC_ALLOC offset 0x%x\n", (int)offset);
	ubc_uobj = uobj;
	ubc_offset = offset;
	reallen = round_page(*lenp);
	ubc_flags = flags;

	ubc_winvalid = 1;

	return RUMP_UBC_MAGIC_WINDOW;
}

void
ubc_release(void *va, int flags)
{

	ubc_winvalid = 0;
}

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo,
	int advice, int flags)
{
	void *win;
	vsize_t len;

	while (todo > 0) {
		len = todo;

		win = ubc_alloc(uobj, uio->uio_offset, &len, 0, flags);
		rump_ubc_magic_uiomove(len, uio);
		ubc_release(win, 0);

		todo -= len;
	}
	return 0;
}


/*
 * Misc routines
 */

void
rumpvm_init()
{

	uvm_vnodeops.pgo_get = vn_get;
	uvm_vnodeops.pgo_put = vn_put;
	aobj_pager.pgo_get = ao_get;
	aobj_pager.pgo_put = ao_put;
	uvmexp.free = 1024*1024; /* XXX */
}

void
uvm_pageactivate(struct vm_page *pg)
{

	/* nada */
}

void
uvm_page_unbusy(struct vm_page **pgs, int npgs)
{

	/* nada */
}

void
uvm_vnp_setsize(struct vnode *vp, voff_t newsize)
{

	vp->v_size = vp->v_writesize = newsize;
}

void
uvm_vnp_setwritesize(struct vnode *vp, voff_t newsize)
{

	vp->v_writesize = newsize;
}

void
uvm_vnp_zerorange(struct vnode *vp, off_t off, size_t len)
{

	/*
	 * XXX: we really really should implement this.
	 * do VOP_GETPAGES + memset
	 */
	return;
}

bool
uvn_clean_p(struct uvm_object *uobj)
{

	return true;
}
