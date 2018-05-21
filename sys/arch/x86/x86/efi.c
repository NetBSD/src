/*	$NetBSD: efi.c,v 1.14.4.1 2018/05/21 04:36:03 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: efi.c,v 1.14.4.1 2018/05/21 04:36:03 pgoyette Exp $");

#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uuid.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <x86/efi.h>

#include <dev/mm.h>
#include <dev/pci/pcivar.h> /* for pci_mapreg_map_enable_decode */

const struct uuid EFI_UUID_ACPI20 = EFI_TABLE_ACPI20;
const struct uuid EFI_UUID_ACPI10 = EFI_TABLE_ACPI10;
const struct uuid EFI_UUID_SMBIOS = EFI_TABLE_SMBIOS;
const struct uuid EFI_UUID_SMBIOS3 = EFI_TABLE_SMBIOS3;

static vaddr_t 	efi_getva(paddr_t);
static void 	efi_relva(vaddr_t);
struct efi_cfgtbl *efi_getcfgtblhead(void);
void 		efi_aprintcfgtbl(void);
void 		efi_aprintuuid(const struct uuid *);
bool 		efi_uuideq(const struct uuid *, const struct uuid *);

static bool efi_is32x64 = false;
static struct efi_systbl *efi_systbl_va = NULL;
static struct efi_cfgtbl *efi_cfgtblhead_va = NULL;
static struct efi_e820memmap {
	struct btinfo_memmap bim;
	struct bi_memmap_entry entry[VM_PHYSSEG_MAX - 1];
} efi_e820memmap;

/*
 * Map a physical address (PA) to a newly allocated virtual address (VA).
 * The VA must be freed using efi_relva().
 */
static vaddr_t
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
static void
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
	} else if (efi_uuideq(uuid, &EFI_UUID_SMBIOS)) {
		aprint_debug(" SMBIOS");
	} else if (efi_uuideq(uuid, &EFI_UUID_SMBIOS3)) {
		aprint_debug(" SMBIOS3");
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

	if (efi_is32x64) {
#if defined(__amd64__)
		struct efi_systbl32 *systbl32 = (void *) efi_systbl_va;
		pa = systbl32->st_cfgtbl;
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) efi_systbl_va;
		if (systbl64->st_cfgtbl & 0xffffffff00000000ULL)
			return NULL;
		pa = (paddr_t) systbl64->st_cfgtbl;
#endif
	} else
		pa = (paddr_t)(u_long) efi_systbl_va->st_cfgtbl;
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
	unsigned long count;

	if (efi_is32x64) {
#if defined(__amd64__)
		struct efi_systbl32 *systbl32 = (void *) efi_systbl_va;
		struct efi_cfgtbl32 *ct32 = (void *) efi_cfgtblhead_va;

		count = systbl32->st_entries;
		aprint_debug("efi: %lu cfgtbl entries:\n", count);
		for (; count; count--, ct32++) {
			aprint_debug("efi: %08" PRIx32, ct32->ct_data);
			efi_aprintuuid(&ct32->ct_uuid);
			aprint_debug("\n");
		}
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) efi_systbl_va;
		struct efi_cfgtbl64 *ct64 = (void *) efi_cfgtblhead_va;
		uint64_t count64 = systbl64->st_entries;

		aprint_debug("efi: %" PRIu64 " cfgtbl entries:\n", count64);
		for (; count64; count64--, ct64++) {
			aprint_debug("efi: %016" PRIx64, ct64->ct_data);
			efi_aprintuuid(&ct64->ct_uuid);
			aprint_debug("\n");
		}
#endif
		return;
	}

	ct = efi_cfgtblhead_va;
	count = efi_systbl_va->st_entries;
	aprint_debug("efi: %lu cfgtbl entries:\n", count);
	for (; count; count--, ct++) {
		aprint_debug("efi: %p", ct->ct_data);
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
	vaddr_t va;

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

	if (efi_is32x64) {
#if defined(__amd64__)
		struct efi_systbl32 *systbl32 = (void *) efi_systbl_va;
		struct efi_cfgtbl32 *ct32 = (void *) efi_cfgtblhead_va;

		count = systbl32->st_entries;
		for (; count; count--, ct32++)
			if (efi_uuideq(&ct32->ct_uuid, uuid))
				return ct32->ct_data;
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) efi_systbl_va;
		struct efi_cfgtbl64 *ct64 = (void *) efi_cfgtblhead_va;
		uint64_t count64 = systbl64->st_entries;

		for (; count64; count64--, ct64++)
			if (efi_uuideq(&ct64->ct_uuid, uuid))
				if (!(ct64->ct_data & 0xffffffff00000000ULL))
					return ct64->ct_data;
#endif
		return 0;	/* Not found. */
	}

	ct = efi_cfgtblhead_va;
	count = efi_systbl_va->st_entries;
	for (; count; count--, ct++)
		if (efi_uuideq(&ct->ct_uuid, uuid))
			return (paddr_t)(u_long) ct->ct_data;

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
	if (sizeof(paddr_t) == 4 &&	/* XXX i386 with PAE */
	    (bi->systblpa & 0xffffffff00000000ULL)) {
		/* Unable to access EFI System Table. */
		return 0;
	}
	if (bi->common.len > 16 && (bi->flags & BI_EFI_32BIT)) {
		/* boot from 32bit UEFI */
#if defined(__amd64__)
		efi_is32x64 = true;
#endif
	} else {
		/* boot from 64bit UEFI */
#if defined(__i386__)
		efi_is32x64 = true;
#endif
	}
	pa = (paddr_t) bi->systblpa;
	return pa;
}

/*
 * Return a pointer to the EFI System Table. The pointer must be freed using
 * efi_relva().
 */
struct efi_systbl *
efi_getsystbl(void)
{
	paddr_t pa;
	vaddr_t va;
	struct efi_systbl *systbl;

	if (efi_systbl_va)
		return efi_systbl_va;

	pa = efi_getsystblpa();
	if (pa == 0)
		return NULL;

	aprint_normal("efi: systbl at pa %" PRIxPADDR "\n", pa);
	va = efi_getva(pa);
	aprint_debug("efi: systbl mapped at va %" PRIxVADDR "\n", va);

	if (efi_is32x64) {
#if defined(__amd64__)
		struct efi_systbl32 *systbl32 = (struct efi_systbl32 *) va;

		/* XXX Check the signature and the CRC32 */
		aprint_debug("efi: signature %" PRIx64 " revision %" PRIx32
		    " crc32 %" PRIx32 "\n", systbl32->st_hdr.th_sig,
		    systbl32->st_hdr.th_rev, systbl32->st_hdr.th_crc32);
		aprint_debug("efi: firmware revision %" PRIx32 "\n",
		    systbl32->st_fwrev);
		/*
		 * XXX Also print fwvendor, which is an UCS-2 string (use
		 * some UTF-16 routine?)
		 */
		aprint_debug("efi: runtime services at pa 0x%08" PRIx32 "\n",
		    systbl32->st_rt);
		aprint_debug("efi: boot services at pa 0x%08" PRIx32 "\n",
		    systbl32->st_bs);

		efi_systbl_va = (struct efi_systbl *) systbl32;
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (struct efi_systbl64 *) va;

		/* XXX Check the signature and the CRC32 */
		aprint_debug("efi: signature %" PRIx64 " revision %" PRIx32
		    " crc32 %" PRIx32 "\n", systbl64->st_hdr.th_sig,
		    systbl64->st_hdr.th_rev, systbl64->st_hdr.th_crc32);
		aprint_debug("efi: firmware revision %" PRIx32 "\n",
		    systbl64->st_fwrev);
		/*
		 * XXX Also print fwvendor, which is an UCS-2 string (use
		 * some UTF-16 routine?)
		 */
		aprint_debug("efi: runtime services at pa 0x%016" PRIx64 "\n",
		    systbl64->st_rt);
		aprint_debug("efi: boot services at pa 0x%016" PRIx64 "\n",
		    systbl64->st_bs);

		efi_systbl_va = (struct efi_systbl *) systbl64;
#endif
		return efi_systbl_va;
	}

	systbl = (struct efi_systbl *) va;
	/* XXX Check the signature and the CRC32 */
	aprint_debug("efi: signature %" PRIx64 " revision %" PRIx32
	    " crc32 %" PRIx32 "\n", systbl->st_hdr.th_sig,
	    systbl->st_hdr.th_rev, systbl->st_hdr.th_crc32);
	aprint_debug("efi: firmware revision %" PRIx32 "\n", systbl->st_fwrev);
	/*
	 * XXX Also print fwvendor, which is an UCS-2 string (use
	 * some UTF-16 routine?)
	 */
	aprint_debug("efi: runtime services at pa %p\n", systbl->st_rt);
	aprint_debug("efi: boot services at pa %p\n", systbl->st_bs);

	efi_systbl_va = systbl;
	return efi_systbl_va;
}

/*
 * EFI is available if we are able to locate the EFI System Table.
 */
void
efi_init(void)
{

	if (efi_getsystbl() == NULL) {
		aprint_debug("efi: missing or invalid systbl\n");
		bootmethod_efi = false;
		return;
	}
	if (efi_getcfgtblhead() == NULL) {
		aprint_debug("efi: missing or invalid cfgtbl\n");
		efi_relva((vaddr_t) efi_systbl_va);
		bootmethod_efi = false;
		return;
	}
	bootmethod_efi = true;
	pci_mapreg_map_enable_decode = true; /* PR port-amd64/53286 */
}

bool
efi_probe(void)
{

	return bootmethod_efi;
}

int
efi_getbiosmemtype(uint32_t type, uint64_t attr)
{

	switch (type) {
	case EFI_MD_TYPE_CODE:
	case EFI_MD_TYPE_DATA:
	case EFI_MD_TYPE_BS_CODE:
	case EFI_MD_TYPE_BS_DATA:
	case EFI_MD_TYPE_FREE:
		return (attr & EFI_MD_ATTR_WB) ? BIM_Memory : BIM_Reserved;

	case EFI_MD_TYPE_RECLAIM:
		return BIM_ACPI;

	case EFI_MD_TYPE_FIRMWARE:
		return BIM_NVS;

	case EFI_MD_TYPE_PMEM:
		return BIM_PMEM;

	case EFI_MD_TYPE_NULL:
	case EFI_MD_TYPE_RT_CODE:
	case EFI_MD_TYPE_RT_DATA:
	case EFI_MD_TYPE_BAD:
	case EFI_MD_TYPE_IOMEM:
	case EFI_MD_TYPE_IOPORT:
	case EFI_MD_TYPE_PALCODE:
	default:
		return BIM_Reserved;
	}
}

const char *
efi_getmemtype_str(uint32_t type)
{
	static const char *efimemtypes[] = {
		"Reserved",
		"LoaderCode",
		"LoaderData",
		"BootServicesCode",
		"BootServicesData",
		"RuntimeServicesCode",
		"RuntimeServicesData",
		"ConventionalMemory",
		"UnusableMemory",
		"ACPIReclaimMemory",
		"ACPIMemoryNVS",
		"MemoryMappedIO",
		"MemoryMappedIOPortSpace",
		"PalCode",
		"PersistentMemory",
	};

	if (type < __arraycount(efimemtypes))
		return efimemtypes[type];
	return "unknown";
}

struct btinfo_memmap *
efi_get_e820memmap(void)
{
	struct btinfo_efimemmap *efimm;
	struct bi_memmap_entry *entry;
	struct efi_md *md;
	uint64_t addr, size;
	uint64_t start_addr = 0;        /* XXX gcc -Os: maybe-uninitialized */
	uint64_t end_addr = 0;          /* XXX gcc -Os: maybe-uninitialized */
	uint32_t i;
	int n, type, seg_type = -1;

	if (efi_e820memmap.bim.common.type == BTINFO_MEMMAP)
		return &efi_e820memmap.bim;

	efimm = lookup_bootinfo(BTINFO_EFIMEMMAP);
	if (efimm == NULL)
		return NULL;

	for (n = 0, i = 0; i < efimm->num; i++) {
		md = (struct efi_md *)(efimm->memmap + efimm->size * i);
		addr = md->md_phys;
		size = md->md_pages * EFI_PAGE_SIZE;
		type = efi_getbiosmemtype(md->md_type, md->md_attr);

#ifdef DEBUG_MEMLOAD
		printf("MEMMAP: p0x%016" PRIx64 "-0x%016" PRIx64
		    ", v0x%016" PRIx64 "-0x%016" PRIx64
		    ", size=0x%016" PRIx64 ", attr=0x%016" PRIx64
		    ", type=%d(%s)\n",
		    addr, addr + size - 1,
		    md->md_virt, md->md_virt + size - 1,
		    size, md->md_attr, md->md_type,
		    efi_getmemtype_str(md->md_type));
#endif

		if (seg_type == -1) {
			/* first entry */
		} else if (seg_type == type && end_addr == addr) {
			/* continuous region */
			end_addr = addr + size;
			continue;
		} else {
			entry = &efi_e820memmap.bim.entry[n];
			entry->addr = start_addr;
			entry->size = end_addr - start_addr;
			entry->type = seg_type;
			if (++n == VM_PHYSSEG_MAX)
				break;
		}

		start_addr = addr;
		end_addr = addr + size;
		seg_type = type;
	}
	if (i > 0 && n < VM_PHYSSEG_MAX) {
		entry = &efi_e820memmap.bim.entry[n];
		entry->addr = start_addr;
		entry->size = end_addr - start_addr;
		entry->type = seg_type;
		++n;
	} else if (n == VM_PHYSSEG_MAX) {
		printf("WARNING: too many memory segments"
		    "(increase VM_PHYSSEG_MAX)\n");
	}

	efi_e820memmap.bim.num = n;
	efi_e820memmap.bim.common.len =
	    (intptr_t)&efi_e820memmap.bim.entry[n] - (intptr_t)&efi_e820memmap;
	efi_e820memmap.bim.common.type = BTINFO_MEMMAP;
	return &efi_e820memmap.bim;
}
