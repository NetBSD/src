/*	$NetBSD: ixp425_pci_space.c,v 1.5.6.1 2006/04/22 11:37:18 simonb Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_pci_space.c,v 1.5.6.1 2006/04/22 11:37:18 simonb Exp $");

/*
 * bus_space PCI functions for ixp425
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

/*
 * Macros to read/write registers
*/
#define CSR_READ_4(x)		*(volatile uint32_t *) \
	(IXP425_PCI_CSR_BASE + (x))
#define CSR_WRITE_4(x, v)	*(volatile uint32_t *) \
	(IXP425_PCI_CSR_BASE + (x)) = (v)

/* Proto types for all the bus_space structure functions */
bs_protos(ixp425_pci);
bs_protos(ixp425_pci_io);
bs_protos(ixp425_pci_mem);
bs_protos(bs_notimpl);

/* special I/O functions */
#if 1	/* XXX */
inline u_int8_t  _pci_io_bs_r_1(void *, bus_space_handle_t, bus_size_t);
inline u_int16_t _pci_io_bs_r_2(void *, bus_space_handle_t, bus_size_t);
inline u_int32_t _pci_io_bs_r_4(void *, bus_space_handle_t, bus_size_t);

inline void _pci_io_bs_w_1(void *, bus_space_handle_t, bus_size_t, u_int8_t);
inline void _pci_io_bs_w_2(void *, bus_space_handle_t, bus_size_t, u_int16_t);
inline void _pci_io_bs_w_4(void *, bus_space_handle_t, bus_size_t, u_int32_t);
#endif

struct bus_space ixp425_pci_bs_tag_template = {
	/* cookie */
	(void *) 0,

	/* mapping/unmapping */
	NULL,
	NULL,
	ixp425_pci_bs_subregion,

	/* allocation/deallocation */
	NULL,
	NULL,

	/* get kernel virtual address */
	NULL,

	/* mmap bus space for userland */
	ixp425_pci_bs_mmap,

	/* barrier */
	ixp425_pci_bs_barrier,

	/* read (single) */
	bs_notimpl_bs_r_1,
	bs_notimpl_bs_r_2,
	bs_notimpl_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	bs_notimpl_bs_rm_1,
	bs_notimpl_bs_rm_2,
	bs_notimpl_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	bs_notimpl_bs_rr_1,
	bs_notimpl_bs_rr_2,
	bs_notimpl_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	bs_notimpl_bs_w_1,
	bs_notimpl_bs_w_2,
	bs_notimpl_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	bs_notimpl_bs_wm_1,
	bs_notimpl_bs_wm_2,
	bs_notimpl_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	bs_notimpl_bs_wr_1,
	bs_notimpl_bs_wr_2,
	bs_notimpl_bs_wr_4,
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	bs_notimpl_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	bs_notimpl_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

void
ixp425_io_bs_init(bus_space_tag_t bs, void *cookie)
{
	*bs = ixp425_pci_bs_tag_template;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp425_pci_io_bs_map;
	bs->bs_unmap = ixp425_pci_io_bs_unmap;
	bs->bs_alloc = ixp425_pci_io_bs_alloc;
	bs->bs_free = ixp425_pci_io_bs_free;
	bs->bs_vaddr = ixp425_pci_io_bs_vaddr;

	/*
	 * IXP425 processor does not have PCI I/O windows
	 */
	/* read (single) */
	bs->bs_r_1 = _pci_io_bs_r_1;
	bs->bs_r_2 = _pci_io_bs_r_2;
	bs->bs_r_4 = _pci_io_bs_r_4;

	/* write (single) */
	bs->bs_w_1 = _pci_io_bs_w_1;
	bs->bs_w_2 = _pci_io_bs_w_2;
	bs->bs_w_4 = _pci_io_bs_w_4;
}

void
ixp425_mem_bs_init(bus_space_tag_t bs, void *cookie)
{
	*bs = ixp425_pci_bs_tag_template;
	bs->bs_cookie = cookie;

	bs->bs_map = ixp425_pci_mem_bs_map;
	bs->bs_unmap = ixp425_pci_mem_bs_unmap;
	bs->bs_alloc = ixp425_pci_mem_bs_alloc;
	bs->bs_free = ixp425_pci_mem_bs_free;
	bs->bs_vaddr = ixp425_pci_mem_bs_vaddr;

	/* read (single) */
	bs->bs_r_1 = ixp425_pci_mem_bs_r_1;
	bs->bs_r_2 = ixp425_pci_mem_bs_r_2;
	bs->bs_r_4 = ixp425_pci_mem_bs_r_4;

	/* write (single) */
	bs->bs_w_1 = ixp425_pci_mem_bs_w_1;
	bs->bs_w_2 = ixp425_pci_mem_bs_w_2;
	bs->bs_w_4 = ixp425_pci_mem_bs_w_4;
}

/* common routine */
int
ixp425_pci_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset,
	bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return (0);
}

void
ixp425_pci_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
	/* NULL */
}	

paddr_t
ixp425_pci_bs_mmap(void *t, bus_addr_t addr, off_t off, int prot, int flags)
{
	/* Not supported. */
	return (-1);
}

/* io bs */
int
ixp425_pci_io_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	int cacheable, bus_space_handle_t *bshp)
{
	*bshp = bpa;
	return (0);
}

void
ixp425_pci_io_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	/* Nothing to do. */
}

int
ixp425_pci_io_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ixp425_pci_io_bs_alloc(): not implemented\n");
}

void
ixp425_pci_io_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp425_pci_io_bs_free(): not implemented\n");
}

void *
ixp425_pci_io_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	/* Not supported. */
	return (NULL);
}

/* special I/O functions */
#if 1	/* _pci_io_bs_{rw}_{124} */
inline u_int8_t
_pci_io_bs_r_1(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	u_int32_t data, n, be;
	int s;

	n = (ioh + off) % 4;
	be = (0xf & ~(1U << n)) << NP_CBE_SHIFT;

	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, be | COMMAND_NP_IO_READ);
	data = CSR_READ_4(PCI_NP_RDATA);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);

	return data >> (8 * n);
}

inline u_int16_t
_pci_io_bs_r_2(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	u_int32_t data, n, be;
	int s;

	n = (ioh + off) % 4;
	be = (0xf & ~((1U << n) | (1U << (n + 1)))) << NP_CBE_SHIFT;

	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, be | COMMAND_NP_IO_READ);
	data = CSR_READ_4(PCI_NP_RDATA);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);

	return data >> (8 * n);
}

inline u_int32_t
_pci_io_bs_r_4(void *v, bus_space_handle_t ioh, bus_size_t off)
{
	u_int32_t data;
	int s;

	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, COMMAND_NP_IO_READ);
	data = CSR_READ_4(PCI_NP_RDATA);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);

	return data;
}

inline void
_pci_io_bs_w_1(void *v, bus_space_handle_t ioh, bus_size_t off,
	u_int8_t val)
{
	u_int32_t data, n, be;
	int s;

	n = (ioh + off) % 4;
	be = (0xf & ~(1U << n)) << NP_CBE_SHIFT;
	data = val << (8 * n);

	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, be | COMMAND_NP_IO_WRITE);
	CSR_WRITE_4(PCI_NP_WDATA, data);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);
}

inline void
_pci_io_bs_w_2(void *v, bus_space_handle_t ioh, bus_size_t off,
	u_int16_t val)
{
	u_int32_t data, n, be;
	int s;

	n = (ioh + off) % 4;
	be = (0xf & ~((1U << n) | (1U << (n + 1)))) << NP_CBE_SHIFT;
	data = val << (8 * n);
	
	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, be | COMMAND_NP_IO_WRITE);
	CSR_WRITE_4(PCI_NP_WDATA, data);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);
}

inline void
_pci_io_bs_w_4(void *v, bus_space_handle_t ioh, bus_size_t off,
	u_int32_t val)
{
	int s;

	PCI_CONF_LOCK(s);
	CSR_WRITE_4(PCI_NP_AD, (ioh + off) & ~3);
	CSR_WRITE_4(PCI_NP_CBE, COMMAND_NP_IO_WRITE);
	CSR_WRITE_4(PCI_NP_WDATA, val);
	if (CSR_READ_4(PCI_ISR) & ISR_PFE)
		CSR_WRITE_4(PCI_ISR, ISR_PFE);
	PCI_CONF_UNLOCK(s);
}
#endif	/* _pci_io_bs_{rw}_{124} */

/* mem bs */
int
ixp425_pci_mem_bs_map(void *t, bus_addr_t bpa, bus_size_t size,
	      int cacheable, bus_space_handle_t *bshp)
{
	const struct pmap_devmap	*pd;

	paddr_t		startpa;
	paddr_t		endpa;
	paddr_t		pa;
	paddr_t		offset;
	vaddr_t		va;
	pt_entry_t	*pte;

	if ((pd = pmap_devmap_find_pa(bpa, size)) != NULL) {
		/* Device was statically mapped. */
		*bshp = pd->pd_va + (bpa - pd->pd_pa);
		return 0;
	}

	endpa = round_page(bpa + size);
	offset = bpa & PAGE_MASK;
	startpa = trunc_page(bpa);

	/* Get some VM.  */
	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return ENOMEM;

	/* Store the bus space handle */
	*bshp = va + offset;

	/* Now map the pages */
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pte = vtopte(va);
		*pte &= ~L2_S_CACHE_MASK;
		PTE_SYNC(pte);
	}
	pmap_update(pmap_kernel());

	return(0);
}

void
ixp425_pci_mem_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t	va;
	vaddr_t	endva;

	if (pmap_devmap_find_va(bsh, size) != NULL) {
		/* Device was statically mapped; nothing to do. */
		return;
	}

	endva = round_page(bsh + size);
	va = trunc_page(bsh);

	pmap_kremove(va, endva - va);
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
}

int
ixp425_pci_mem_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("ixp425_mem_bs_alloc(): not implemented\n");
}

void    
ixp425_pci_mem_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("ixp425_mem_bs_free(): not implemented\n");
}

void *
ixp425_pci_mem_bs_vaddr(void *t, bus_space_handle_t bsh)
{
	return ((void *)bsh);
}
/* End of ixp425_pci_space.c */
