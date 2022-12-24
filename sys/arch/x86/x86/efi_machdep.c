/*	$NetBSD: efi_machdep.c,v 1.4 2022/12/24 15:23:02 andvar Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: efi_machdep.c,v 1.4 2022/12/24 15:23:02 andvar Exp $");

#include "efi.h"
#include "opt_efi.h"

#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uuid.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <machine/pmap_private.h>

#include <x86/bus_defs.h>
#include <x86/bus_funcs.h>
#include <x86/efi.h>
#include <x86/fpu.h>

#include <dev/mm.h>
#if NPCI > 0
#include <dev/pci/pcivar.h> /* for pci_mapreg_map_enable_decode */
#endif

const struct uuid EFI_UUID_ACPI20 = EFI_TABLE_ACPI20;
const struct uuid EFI_UUID_ACPI10 = EFI_TABLE_ACPI10;
const struct uuid EFI_UUID_SMBIOS = EFI_TABLE_SMBIOS;
const struct uuid EFI_UUID_SMBIOS3 = EFI_TABLE_SMBIOS3;

static vaddr_t	efi_getva(paddr_t);
static void	efi_relva(paddr_t, vaddr_t);
struct efi_cfgtbl *efi_getcfgtblhead(void);
void		efi_aprintcfgtbl(void);
void		efi_aprintuuid(const struct uuid *);
bool		efi_uuideq(const struct uuid *, const struct uuid *);

static bool efi_is32x64 = false;
static paddr_t efi_systbl_pa;
static struct efi_systbl *efi_systbl_va = NULL;
static struct efi_cfgtbl *efi_cfgtblhead_va = NULL;
static struct efi_e820memmap {
	struct btinfo_memmap bim;
	struct bi_memmap_entry entry[VM_PHYSSEG_MAX - 1];
} efi_e820memmap;

#ifdef EFI_RUNTIME

#include <dev/efivar.h>

#include <uvm/uvm_extern.h>

#if !(NEFI > 0)
#error options EFI_RUNTIME makes no sense without pseudo-device efi.
#endif

struct pmap *efi_runtime_pmap __read_mostly;

static kmutex_t efi_runtime_lock __cacheline_aligned;
static struct efi_rt efi_rt __read_mostly;
static struct efi_ops efi_runtime_ops __read_mostly;

static void efi_runtime_init(void);

#endif

/*
 * Map a physical address (PA) to a newly allocated virtual address (VA).
 * The VA must be freed using efi_relva().
 */
static vaddr_t
efi_getva(paddr_t pa)
{
	vaddr_t va;
	int rv;

	rv = _x86_memio_map(x86_bus_space_mem, pa,
	    PAGE_SIZE, 0, (bus_space_handle_t *)&va);
	if (rv != 0) {
		aprint_debug("efi: unable to allocate va\n");
		return 0;
	}

	return va;
}

/*
 * Free a virtual address (VA) allocated using efi_getva().
 */
static void
efi_relva(paddr_t pa, vaddr_t va)
{
	(void)_x86_memio_unmap(x86_bus_space_mem, (bus_space_handle_t)va,
	    PAGE_SIZE, NULL);
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
	aprint_debug("%02" PRIx8 "", uuid->clock_seq_low);
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
	efi_systbl_pa = pa;
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
		efi_relva(efi_systbl_pa, (vaddr_t) efi_systbl_va);
		bootmethod_efi = false;
		return;
	}
	bootmethod_efi = true;
#if NPCI > 0
	pci_mapreg_map_enable_decode = true; /* PR port-amd64/53286 */
#endif

#ifdef EFI_RUNTIME
	efi_runtime_init();
#endif
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

#ifdef EFI_RUNTIME

/*
 * XXX move to sys/dev/efi/efi.h
 */
#ifdef _LP64
#define	EFIERR(x)	(0x8000000000000000ul | (x))
#else
#define	EFIERR(x)	(0x80000000ul | (x))
#endif

#define	EFI_UNSUPPORTED		EFIERR(3)
#define	EFI_DEVICE_ERROR	EFIERR(7)

/*
 * efi_runtime_init()
 *
 *	Set up kernel access to EFI runtime services:
 *
 *	- Create efi_runtime_pmap.
 *	- Enter all the EFI runtime memory mappings into it.
 *	- Make a copy of the EFI runtime services table in efi_rt.
 *	- Initialize efi_runtime_lock to serialize calls.
 *	- Register EFI runtime service operations for /dev/efi.
 *
 *	On failure, leaves efi_rt zero-initialized and everything else
 *	uninitialized.
 */
static void
efi_runtime_init(void)
{
	struct efi_systbl *systbl;
	struct btinfo_efimemmap *efimm;
	uint32_t i;
	int error;

	/*
	 * Refuse to handle EFI runtime services with cross-word-sizes
	 * for now.  We would need logic to handle the cross table
	 * types, and logic to translate between the calling
	 * conventions -- might be easy for 32-bit EFI and 64-bit OS,
	 * but sounds painful to contemplate for 64-bit EFI and 32-bit
	 * OS.
	 */
	if (efi_is32x64) {
		aprint_debug("%s: 32x64 runtime services not supported\n",
		    __func__);
		return;
	}

	/*
	 * Verify that we have an EFI system table with runtime
	 * services and an EFI memory map.
	 */
	systbl = efi_getsystbl();
	if (systbl->st_rt == NULL) {
		aprint_debug("%s: no runtime\n", __func__);
		return;
	}
	if ((efimm = lookup_bootinfo(BTINFO_EFIMEMMAP)) == NULL) {
		aprint_debug("%s: no efi memmap\n", __func__);
		return;
	}

	/*
	 * Create a pmap for EFI runtime services and switch to it to
	 * enter all of the mappings needed for EFI runtime services
	 * according to the EFI_MEMORY_DESCRIPTOR records.
	 */
	efi_runtime_pmap = pmap_create();
	void *const cookie = pmap_activate_sync(efi_runtime_pmap);
	for (i = 0; i < efimm->num; i++) {
		struct efi_md *md = (void *)(efimm->memmap + efimm->size * i);
		uint64_t j;
		vaddr_t va;
		paddr_t pa;
		int prot, flags;

		/*
		 * Only enter mappings tagged EFI_MEMORY_RUNTIME.
		 * Ignore all others.
		 */
		if ((md->md_attr & EFI_MD_ATTR_RT) == 0)
			continue;

		/*
		 * For debug boots, print the memory descriptor.
		 */
		aprint_debug("%s: map %zu pages at %#"PRIxVADDR
		    " to %#"PRIxPADDR" type %"PRIu32" attrs 0x%08"PRIx64"\n",
		    __func__, (size_t)md->md_pages, (vaddr_t)md->md_virt,
		    (paddr_t)md->md_phys, md->md_type, md->md_attr);

		/*
		 * Allow read and write access in all of the mappings.
		 * For code mappings, also allow execution by default.
		 *
		 * Even code mappings must be writable, apparently.
		 * The mappings can be marked RO or XP to prevent write
		 * or execute, but the code mappings are usually at the
		 * level of entire PECOFF objects containing both rw-
		 * and r-x sections.  The EFI_MEMORY_ATTRIBUTES_TABLE
		 * provides finer-grained mapping protections, but we
		 * don't currently use it.
		 *
		 * XXX Should parse EFI_MEMORY_ATTRIBUTES_TABLE and use
		 * it to nix W or X access when possible.
		 */
		prot = VM_PROT_READ|VM_PROT_WRITE;
		switch (md->md_type) {
		case EFI_MD_TYPE_RT_CODE:
			prot |= VM_PROT_EXECUTE;
			break;
		}

		/*
		 * Additionally pass on:
		 *
		 *	EFI_MEMORY_UC (uncacheable) -> PMAP_NOCACHE
		 *	EFI_MEMORY_WC (write-combining) -> PMAP_WRITE_COMBINE
		 *	EFI_MEMORY_RO (read-only) -> clear VM_PROT_WRITE
		 *	EFI_MEMORY_XP (exec protect) -> clear VM_PROT_EXECUTE
		 */
		flags = 0;
		if (md->md_attr & EFI_MD_ATTR_UC)
			flags |= PMAP_NOCACHE;
		else if (md->md_attr & EFI_MD_ATTR_WC)
			flags |= PMAP_WRITE_COMBINE;
		if (md->md_attr & EFI_MD_ATTR_RO)
			prot &= ~VM_PROT_WRITE;
		if (md->md_attr & EFI_MD_ATTR_XP)
			prot &= ~VM_PROT_EXECUTE;

		/*
		 * Get the physical address, and the virtual address
		 * that the EFI runtime services want mapped to it.
		 *
		 * If the requested virtual address is zero, assume
		 * we're using physical addressing, i.e., VA is the
		 * same as PA.
		 *
		 * This logic is intended to allow the bootloader to
		 * choose whether to use physical addressing or to use
		 * virtual addressing with RT->SetVirtualAddressMap --
		 * the kernel should work either way (although as of
		 * time of writing it has only been tested with
		 * physical addressing).
		 */
		pa = md->md_phys;
		va = md->md_virt;
		if (va == 0)
			va = pa;

		/*
		 * Fail if EFI runtime services want any virtual pages
		 * of the kernel map.
		 */
		if (VM_MIN_KERNEL_ADDRESS <= va &&
		    va < VM_MAX_KERNEL_ADDRESS) {
			aprint_debug("%s: efi runtime overlaps kernel map"
			    " %"PRIxVADDR" in [%"PRIxVADDR", %"PRIxVADDR")\n",
			    __func__,
			    va,
			    (vaddr_t)VM_MIN_KERNEL_ADDRESS,
			    (vaddr_t)VM_MAX_KERNEL_ADDRESS);
			goto fail;
		}

		/*
		 * Fail if it would interfere with a direct map.
		 *
		 * (It's possible that it might happen to be identical
		 * to the direct mapping, in which case we could skip
		 * this entry.  Seems unlikely; let's deal with that
		 * edge case as it comes up.)
		 */
#ifdef __HAVE_DIRECT_MAP
		if (PMAP_DIRECT_BASE <= va && va < PMAP_DIRECT_END) {
			aprint_debug("%s: efi runtime overlaps direct map"
			    " %"PRIxVADDR" in [%"PRIxVADDR", %"PRIxVADDR")\n",
			    __func__,
			    va,
			    (vaddr_t)PMAP_DIRECT_BASE,
			    (vaddr_t)PMAP_DIRECT_END);
			goto fail;
		}
#endif

		/*
		 * Enter each page in the range of this memory
		 * descriptor into efi_runtime_pmap.
		 */
		for (j = 0; j < md->md_pages; j++) {
			error = pmap_enter(efi_runtime_pmap,
			    va + j*PAGE_SIZE, pa + j*PAGE_SIZE, prot, flags);
			KASSERTMSG(error == 0, "error=%d", error);
		}
	}

	/*
	 * Commit the updates, make a copy of the EFI runtime services
	 * for easy determination of unsupported ones without needing
	 * the pmap, and deactivate the pmap now that we're done with
	 * it for now.
	 */
	pmap_update(efi_runtime_pmap);
	memcpy(&efi_rt, systbl->st_rt, sizeof(efi_rt));
	pmap_deactivate_sync(efi_runtime_pmap, cookie);

	/*
	 * Initialize efi_runtime_lock for serializing access to the
	 * EFI runtime services from any context up to interrupts at
	 * IPL_VM.
	 */
	mutex_init(&efi_runtime_lock, MUTEX_DEFAULT, IPL_VM);

	/*
	 * Register the EFI runtime operations for /dev/efi.
	 */
	efi_register_ops(&efi_runtime_ops);

	return;

fail:	/*
	 * On failure, deactivate and destroy efi_runtime_pmap -- no
	 * runtime services.
	 */
	pmap_deactivate_sync(efi_runtime_pmap, cookie);
	pmap_destroy(efi_runtime_pmap);
	efi_runtime_pmap = NULL;
	/*
	 * efi_rt is all zero, so will lead to EFI_UNSUPPORTED even if
	 * used outside efi_runtime_ops (which is now not registered)
	 */
}

struct efi_runtime_cookie {
	void	*erc_pmap_cookie;
};

/*
 * efi_runtime_enter(cookie)
 *
 *	Prepare to call an EFI runtime service, storing state for the
 *	context in cookie.  Caller must call efi_runtime_exit when
 *	done.
 */
static void
efi_runtime_enter(struct efi_runtime_cookie *cookie)
{

	KASSERT(efi_runtime_pmap != NULL);

	/*
	 * Serialize queries to the EFI runtime services.
	 *
	 * The UEFI spec allows some concurrency among them with rules
	 * about which calls can run in parallel with which other
	 * calls, but it is simplest if we just serialize everything --
	 * none of this is performance-critical.
	 */
	mutex_enter(&efi_runtime_lock);

	/*
	 * EFI runtime services may use the FPU, so stash any user FPU
	 * state and enable kernel use of it.  This has the side
	 * effects of disabling preemption and of blocking interrupts
	 * at up to and including IPL_VM.
	 */
	fpu_kern_enter();

	/*
	 * Activate the efi_runtime_pmap so that the EFI runtime
	 * services have access to the memory mappings the firmware
	 * requested, but not access to any user mappings.  They still,
	 * however, have access to all kernel mappings, so we can pass
	 * in pointers to buffers in KVA -- the EFI runtime services
	 * run privileged, which they need in order to do I/O anyway.
	 */
	cookie->erc_pmap_cookie = pmap_activate_sync(efi_runtime_pmap);
}

/*
 * efi_runtime_exit(cookie)
 *
 *	Restore state prior to efi_runtime_enter as stored in cookie
 *	for a call to an EFI runtime service.
 */
static void
efi_runtime_exit(struct efi_runtime_cookie *cookie)
{

	pmap_deactivate_sync(efi_runtime_pmap, cookie->erc_pmap_cookie);
	fpu_kern_leave();
	mutex_exit(&efi_runtime_lock);
}

/*
 * efi_runtime_gettime(tm, tmcap)
 *
 *	Call RT->GetTime, or return EFI_UNSUPPORTED if unsupported.
 */
static efi_status
efi_runtime_gettime(struct efi_tm *tm, struct efi_tmcap *tmcap)
{
	efi_status status;
	struct efi_runtime_cookie cookie;

	if (efi_rt.rt_gettime == NULL)
		return EFI_UNSUPPORTED;

	efi_runtime_enter(&cookie);
	status = efi_rt.rt_gettime(tm, tmcap);
	efi_runtime_exit(&cookie);

	return status;
}


/*
 * efi_runtime_settime(tm)
 *
 *	Call RT->SetTime, or return EFI_UNSUPPORTED if unsupported.
 */
static efi_status
efi_runtime_settime(struct efi_tm *tm)
{
	efi_status status;
	struct efi_runtime_cookie cookie;

	if (efi_rt.rt_settime == NULL)
		return EFI_UNSUPPORTED;

	efi_runtime_enter(&cookie);
	status = efi_rt.rt_settime(tm);
	efi_runtime_exit(&cookie);

	return status;
}

/*
 * efi_runtime_getvar(name, vendor, attrib, datasize, data)
 *
 *	Call RT->GetVariable.
 */
static efi_status
efi_runtime_getvar(efi_char *name, struct uuid *vendor, uint32_t *attrib,
    unsigned long *datasize, void *data)
{
	efi_status status;
	struct efi_runtime_cookie cookie;

	if (efi_rt.rt_getvar == NULL)
		return EFI_UNSUPPORTED;

	efi_runtime_enter(&cookie);
	status = efi_rt.rt_getvar(name, vendor, attrib, datasize, data);
	efi_runtime_exit(&cookie);

	return status;
}

/*
 * efi_runtime_nextvar(namesize, name, vendor)
 *
 *	Call RT->GetNextVariableName.
 */
static efi_status
efi_runtime_nextvar(unsigned long *namesize, efi_char *name,
    struct uuid *vendor)
{
	efi_status status;
	struct efi_runtime_cookie cookie;

	if (efi_rt.rt_scanvar == NULL)
		return EFI_UNSUPPORTED;

	efi_runtime_enter(&cookie);
	status = efi_rt.rt_scanvar(namesize, name, vendor);
	efi_runtime_exit(&cookie);

	return status;
}

/*
 * efi_runtime_setvar(name, vendor, attrib, datasize, data)
 *
 *	Call RT->SetVariable.
 */
static efi_status
efi_runtime_setvar(efi_char *name, struct uuid *vendor, uint32_t attrib,
    unsigned long datasize, void *data)
{
	efi_status status;
	struct efi_runtime_cookie cookie;

	if (efi_rt.rt_setvar == NULL)
		return EFI_UNSUPPORTED;

	efi_runtime_enter(&cookie);
	status = efi_rt.rt_setvar(name, vendor, attrib, datasize, data);
	efi_runtime_exit(&cookie);

	return status;
}

static struct efi_ops efi_runtime_ops = {
	.efi_gettime = efi_runtime_gettime,
	.efi_settime = efi_runtime_settime,
	.efi_getvar = efi_runtime_getvar,
	.efi_setvar = efi_runtime_setvar,
	.efi_nextvar = efi_runtime_nextvar,
};

#endif	/* EFI_RUNTIME */
