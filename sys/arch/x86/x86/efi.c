/*	$NetBSD: efi.c,v 1.2.2.4 2016/10/05 20:55:37 skrll Exp $	*/
/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: efi.c,v 1.2.2.4 2016/10/05 20:55:37 skrll Exp $");
#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uuid.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <x86/efi.h>

#include <dev/mm.h>

const struct uuid EFI_UUID_ACPI20 = {
	.time_low = 0x8868e871,
	.time_mid = 0xe4f1,
	.time_hi_and_version = 0x11d3,
	.clock_seq_hi_and_reserved = 0xbc,
	.clock_seq_low = 0x22,
	.node = {0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}
};
const struct uuid EFI_UUID_ACPI10 = {
	.time_low = 0xeb9d2d30,
	.time_mid = 0x2d88,
	.time_hi_and_version = 0x11d3,
	.clock_seq_hi_and_reserved = 0x9a,
	.clock_seq_low = 0x16,
	.node = {0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d}
};

vaddr_t 	efi_getva(paddr_t);
void 		efi_relva(vaddr_t);
struct efi_cfgtbl *efi_getcfgtblhead(void);
void 		efi_aprintcfgtbl(void);
void 		efi_aprintuuid(const struct uuid *);
bool 		efi_uuideq(const struct uuid *, const struct uuid *);

static struct efi_systbl *efi_systbl_va = NULL;
static struct efi_cfgtbl *efi_cfgtblhead_va = NULL;

/*
 * Map a physical address (PA) to a newly allocated virtual address (VA).
 * The VA must be freed using efi_relva().
 */
vaddr_t
efi_getva(paddr_t pa)
{
	vaddr_t va;

#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	mm_md_direct_mapped_phys(pa, &va);
#else
	/* XXX This code path is not tested. */
	va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	if (va == 0) {
		aprint_debug("efi: unable to allocate va\n");
		return 0;
	}
	pmap_kenter_pa(va, pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());
#endif
	return va;
}

/*
 * Free a virtual address (VA) allocated using efi_getva().
 */
void
efi_relva(vaddr_t va)
{
#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	/* XXX Should we free the va? */
#else
	/* XXX This code path is not tested. */
	uvm_km_free(kernel_map, va, PAGE_SIZE, UVM_KMF_VAONLY);
#endif
}

/*
 * Test if 2 UUIDs matches.
 */
bool
efi_uuideq(const struct uuid * a, const struct uuid * b)
{
	return !memcmp(a, b, sizeof(struct uuid));
}

/*
 * Print an UUID in a human-readable manner.
 */
void
efi_aprintuuid(const struct uuid * uuid)
{
	int i;

	aprint_debug(" %08" PRIx32 "", uuid->time_low);
	aprint_debug("-%04" PRIx16 "", uuid->time_mid);
	aprint_debug("-%04" PRIx16 "", uuid->time_hi_and_version);
	aprint_debug("-%02" PRIx8 "", uuid->clock_seq_hi_and_reserved);
	aprint_debug("-%02" PRIx8 "", uuid->clock_seq_low);
	aprint_debug("-");
	for (i = 0; i < _UUID_NODE_LEN; i++) {
		aprint_debug("%02" PRIx8 "", uuid->node[i]);
	}
	/* If known, also print the human-readable name */
	if (efi_uuideq(uuid, &EFI_UUID_ACPI20)) {
		aprint_debug(" ACPI 2.0");
	} else if (efi_uuideq(uuid, &EFI_UUID_ACPI10)) {
		aprint_debug(" ACPI 1.0");
	}
}

/*
 * Return the VA of the cfgtbl. Must be freed using efi_relva().
 */
struct efi_cfgtbl *
efi_getcfgtblhead(void)
{
	paddr_t	pa;
	vaddr_t	va;

	if (efi_cfgtblhead_va != NULL)
		return efi_cfgtblhead_va;

	pa = (paddr_t) efi_systbl_va->st_cfgtbl;
	aprint_debug("efi: cfgtbl at pa %" PRIxPADDR "\n", pa);
	va = efi_getva(pa);
	aprint_debug("efi: cfgtbl mapped at va %" PRIxVADDR "\n", va);
	efi_cfgtblhead_va = (struct efi_cfgtbl *) va;
	efi_aprintcfgtbl();

	return efi_cfgtblhead_va;
}

/*
 * Print the config tables.
 */
void
efi_aprintcfgtbl(void)
{
	struct efi_cfgtbl *ct;
	unsigned long 	count;

	ct = efi_cfgtblhead_va;
	count = efi_systbl_va->st_entries;
	aprint_debug("efi: %lu cfgtbl entries:\n", count);
	for (; count; count--, ct++) {
		aprint_debug("efi: %16" PRIx64 "", ct->ct_data);
		efi_aprintuuid(&ct->ct_uuid);
		aprint_debug("\n");
	}
}

/*
 * Return the VA of the config table with the given UUID if found.
 * The VA must be freed using efi_relva().
 */
void *
efi_getcfgtbl(const struct uuid * uuid)
{
	paddr_t pa;
	vaddr_t  va;

	pa = efi_getcfgtblpa(uuid);
	if (pa == 0)
		return NULL;
	va = efi_getva(pa);
	return (void *) va;
}

/*
 * Return the PA of the first config table.
 */
paddr_t
efi_getcfgtblpa(const struct uuid * uuid)
{
	struct efi_cfgtbl *ct;
	unsigned long count;

	ct = efi_cfgtblhead_va;
	count = efi_systbl_va->st_entries;
	for (; count; count--, ct++)
		if (efi_uuideq(&ct->ct_uuid, uuid))
			return (paddr_t) ct->ct_data;

	return 0;	/* Not found. */
}

/* Return the PA of the EFI System Table. */
paddr_t
efi_getsystblpa(void)
{
	struct btinfo_efi *bi;
	paddr_t	pa;

	bi = lookup_bootinfo(BTINFO_EFI);
	if (bi == NULL) {
		/* Unable to locate the EFI System Table. */
		return 0;
	}
	pa = bi->bi_systbl;
	return pa;
}

/*
 * Return a pointer to the EFI System Table. The pointer must be freed using
 * efi_relva().
 */
struct efi_systbl *
efi_getsystbl(void)
{
	if (!efi_systbl_va) {
		paddr_t 	pa;
		vaddr_t 	va;
		struct efi_systbl *systbl;

		pa = efi_getsystblpa();
		if (pa == 0)
			return NULL;
		aprint_normal("efi: systbl at pa %" PRIxPADDR "\n", pa);
		va = efi_getva(pa);
		aprint_debug("efi: systbl mapped at va %" PRIxVADDR "\n", va);
		systbl = (struct efi_systbl *) va;
		/* XXX Check the signature and the CRC32 */
		aprint_debug("efi: signature %" PRIx64 " revision %" PRIx32
		    " crc32 %" PRIx32 "\n", systbl->st_hdr.th_sig,
		    systbl->st_hdr.th_rev, systbl->st_hdr.th_crc32);
		aprint_debug("efi: firmware revision %" PRIx32 "\n",
		    systbl->st_fwrev);
		/*
		 * XXX Also print fwvendor, which is an UCS-2 string (use
		 * some UTF-16 routine?)
		 */
		aprint_debug("efi: runtime services at pa %" PRIx64 "\n",
		    systbl->st_rt);
		aprint_debug("efi: boot services at pa %p\n",
		    systbl->st_bs);
		efi_systbl_va = systbl;
	}
	return efi_systbl_va;
}

/*
 * EFI is available if we are able to locate the EFI System Table.
 */
bool
efi_probe(void)
{
	if (efi_getsystbl() == 0) {
		aprint_debug("efi: missing or invalid systbl\n");
		return false;
	}
	if (efi_getcfgtblhead() == 0) {
		aprint_debug("efi: missing or invalid cfgtbl\n");
		efi_relva((vaddr_t) efi_systbl_va);
		return false;
	}
	return true;
}
