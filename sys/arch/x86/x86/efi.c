/*	$NetBSD: efi.c,v 1.22 2021/10/07 12:52:27 msaitoh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: efi.c,v 1.22 2021/10/07 12:52:27 msaitoh Exp $");

#include <sys/kmem.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/uuid.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <machine/pmap.h>
#include <x86/bus_defs.h>
#include <x86/bus_funcs.h>
#include <x86/efi.h>
#include <x86/cpufunc.h>

#include <dev/mm.h>
#if NPCI > 0
#include <dev/pci/pcivar.h> /* for pci_mapreg_map_enable_decode */
#endif

const struct uuid EFI_UUID_ACPI20 = EFI_TABLE_ACPI20;
const struct uuid EFI_UUID_ACPI10 = EFI_TABLE_ACPI10;
const struct uuid EFI_UUID_SMBIOS = EFI_TABLE_SMBIOS;
const struct uuid EFI_UUID_SMBIOS3 = EFI_TABLE_SMBIOS3;
const struct uuid EFI_UUID_ESRT = EFI_TABLE_ESRT;

static vaddr_t	efi_getva(paddr_t);
static void	efi_relva(paddr_t, vaddr_t);
struct efi_cfgtbl *efi_getcfgtblhead(void);
void		efi_aprintcfgtbl(void);
void		efi_aprintuuid(const struct uuid *);
bool		efi_uuideq(const struct uuid *, const struct uuid *);


static struct efi_systbl *ST = NULL;
static struct efi_rt *RT = NULL;

static kmutex_t efi_lock;
struct pmap *efi_pm;
u_long efi_psw;

static bool efi_is32x64 = false;
static paddr_t efi_systbl_pa;
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
	} else if (efi_uuideq(uuid, &EFI_UUID_ESRT)) {
		aprint_debug(" ESRT");
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
		struct efi_systbl32 *systbl32 = (void *) ST;
		pa = systbl32->st_cfgtbl;
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) ST;
		if (systbl64->st_cfgtbl & 0xffffffff00000000ULL)
			return NULL;
		pa = (paddr_t) systbl64->st_cfgtbl;
#endif
	} else
		pa = (paddr_t)(u_long) ST->st_cfgtbl;
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
		struct efi_systbl32 *systbl32 = (void *) ST;
		struct efi_cfgtbl32 *ct32 = (void *) efi_cfgtblhead_va;

		count = systbl32->st_entries;
		aprint_debug("efi: %lu cfgtbl entries:\n", count);
		for (; count; count--, ct32++) {
			aprint_debug("efi: %08" PRIx32, ct32->ct_data);
			efi_aprintuuid(&ct32->ct_uuid);
			aprint_debug("\n");
		}
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) ST;
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
	count = ST->st_entries;
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
		struct efi_systbl32 *systbl32 = (void *) ST;
		struct efi_cfgtbl32 *ct32 = (void *) efi_cfgtblhead_va;

		count = systbl32->st_entries;
		for (; count; count--, ct32++)
			if (efi_uuideq(&ct32->ct_uuid, uuid))
				return ct32->ct_data;
#elif defined(__i386__)
		struct efi_systbl64 *systbl64 = (void *) ST;
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
	count = ST->st_entries;
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

	if (ST)
		return ST;

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

		ST = (struct efi_systbl *) systbl32;
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

		ST = (struct efi_systbl *) systbl64;
#endif
		return ST;
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

	ST = systbl;
	return ST;
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
		efi_relva(efi_systbl_pa, (vaddr_t) ST);
		bootmethod_efi = false;
		return;
	}
	if (ST->st_rt == NULL) {
		aprint_debug("efi: missing or invalid runtime services table\n");
		efi_relva(efi_systbl_pa, (vaddr_t) ST);
		bootmethod_efi = false;
		return;
	}
	bootmethod_efi = true;

	//mutex_init(&efi_lock, MUTEX_DEFAULT, IPL_HIGH);

	aprint_normal("efi: systbl mapped at va %p\n", (void *)ST);

	aprint_normal("RT services at: %p\n", (void *)RT);
	aprint_normal("RT gettime address = %p\n", (void *)((uint64_t)RT + sizeof(struct efi_tblhdr)));

	// if (efi_rt_init()) {
	// 	aprint_error("efi: error initialising runtime services\n");
	// 	return;
	// }

	// aprint_normal("\nafter efirt_init\n");
	// aprint_normal("efi: systbl mapped at va %p\n", (void *)ST);
	// aprint_normal("RT services at: %p\n", (void *)RT);
	// aprint_normal("RT gettime = %p\n", (void *)((uint64_t)RT + sizeof(struct efi_tblhdr)));

	RT = (void *) efi_getva((paddr_t) ST->st_rt);
	ST->st_rt = RT;

	aprint_normal("\nafter allocating virt memory\n");
	aprint_normal("efi: systbl mapped at va %p\n", (void *)ST);
	aprint_normal("RT services at: %p\n", (void *)RT);
	aprint_normal("RT gettime = %p\n", (void *)RT->rt_gettime);

	/*
	 * DEBUG START
	 */

	efi_status status;

	struct efi_tm *efi_debug_time = NULL;

	//vaddr_t rt_pmap = PMAP_DIRECT_MAP((paddr_t) RT);

	//aprint_normal("RT pmap = %p\n", (void *)rt_pmap);

	status = efi_rt_gettime(efi_debug_time, NULL);

	if (status == 0)
		aprint_normal("EFI time: %04x-%01x-%01x\n", efi_debug_time->tm_year, efi_debug_time->tm_mon, efi_debug_time->tm_mday);

	/*
	 * DEBUG END
	 */

	efi_print_esrt();

#if NPCI > 0
	pci_mapreg_map_enable_decode = true; /* PR port-amd64/53286 */
#endif
}

/*
 * Prints ESRT contents to dmesg when booting in debug mode
 * `boot -x` 
 */
void
efi_print_esrt(void)
{
	const struct uuid esrt_uuid = (const struct uuid) EFI_TABLE_ESRT;

	struct efi_esrt_table *esrt = NULL;
	struct efi_esrt_entry_v1 *esrt_entries;

	esrt = (struct efi_esrt_table *) efi_getcfgtbl(&esrt_uuid);

	if (esrt == NULL || esrt == 0) {
		aprint_error("ESRT Couldn't find esrt on the system\n");
		return;
	}

	aprint_debug("ESRT Fw Resource Count = %d\n", esrt->fw_resource_count);
	aprint_debug("ESRT Fw Max Resource Count = %d\n", esrt->fw_resource_count_max);
	aprint_debug("ESRT Fw Resource Version = %ld\n", esrt->fw_resource_version);

	esrt_entries = (struct efi_esrt_entry_v1 *) esrt->entries;

	for (int i = 0; i < esrt->fw_resource_count; ++i) {
		const struct efi_esrt_entry_v1 *e = &esrt_entries[i];

		aprint_debug("ESRT[%d]:\n", i);
		aprint_debug("	Fw Class:");
		efi_aprintuuid(&e->fw_class);
		aprint_debug("\n");
		aprint_debug("  Fw Type: 0x%08x\n", e->fw_type);
		aprint_debug("  Fw Version: 0x%08x\n", e->fw_version);
		aprint_debug("  Lowest Supported Fw Version: 0x%08x\n", e->lowest_supported_fw_version);
		aprint_debug("  Capsule Flags: 0x%08x\n", e->capsule_flags);
		aprint_debug("  Last Attempt Version: 0x%08x\n", e->last_attempt_version);
		aprint_debug("  Last Attempt Status: 0x%08x\n", e->last_attempt_status);
	}
}

bool
efi_probe(void)
{

	return bootmethod_efi;
}

int
efi_rt_init(void)
{
	const size_t sz = PAGE_SIZE * 2;
	vaddr_t va, cva;
	paddr_t cpa;

	va = uvm_km_alloc(kernel_map, sz, 0, UVM_KMF_VAONLY);
	if (va == 0) {
		aprint_error("%s: can't allocate VA\n", __func__);
		return ENOMEM;
	}

	for (cva = va, cpa = trunc_page(efi_systbl_pa);
		 cva < va + sz;
		 cva += PAGE_SIZE, cpa += PAGE_SIZE) {
		pmap_kenter_pa(cva, cpa, VM_PROT_READ, 0);
	}
	pmap_update(pmap_kernel());

	ST = (void *)(va + (efi_systbl_pa - trunc_page(efi_systbl_pa)));
	if (ST->st_hdr.th_sig != EFI_SYSTBL_SIG) {
		aprint_error("EFI: signature mismatch (%#lx != %#lx)\n",
		    ST->st_hdr.th_sig, EFI_SYSTBL_SIG);
		return EINVAL;
	}

	RT = ST->st_rt;
	mutex_init(&efi_lock, MUTEX_DEFAULT, IPL_HIGH);

	return 0;
}

// void
// efi_map_runtime(void)
// {
// 	EFI_STATUS status;
// 	EFI_MEMORY_DESCRIPTOR *desc;
// 	UINTN NoEntries, MapKey, DescriptorSize;
// 	UINT32 DescriptorVersion;
// 	size_t allocsz;

// 	EFI_MEMORY_DESCRIPTOR *desc;
// 	int i;

// 	efi_get_e820memmap();

// 	// desc = efi_memory_get_map(&NoEntries, &MapKey, &DescriptorSize,
// 	//     &DescriptorVersion, true);

// 	// struct btinfo_memmap *mmap = efi_get_e820memmap();

// 	// uint32_t mmap_desc_size = mmap->mmap_desc_size; //
// 	// uint32_t mmap_size = bios_efiinfo->mmap_size; //
// 	// uint64_t mmap_start = bios_efiinfo->mmap_start; // TODO
// }

void
efi_rt_map_range(vaddr_t va, paddr_t pa, size_t sz, enum efi_rt_mem_type mem_type)
{
	int flags = 0;
	int prot = 0;

	switch (mem_type) {
	case ARM_EFIRT_MEM_CODE:
		/* need write permission because fw devs */
		prot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
		break;
	case ARM_EFIRT_MEM_DATA:
		prot = VM_PROT_READ | VM_PROT_WRITE;
		break;
	case ARM_EFIRT_MEM_MMIO:
		prot = VM_PROT_READ | VM_PROT_WRITE;
		flags = 0x20000000;
		break;
	default:
		panic("%s: unsupported type %d", __func__, mem_type);
	}
	if (va >= VM_MAXUSER_ADDRESS || va >= VM_MAXUSER_ADDRESS - sz) {
		printf("Incorrect EFI mapping range %" PRIxVADDR
		    "- %" PRIxVADDR "\n", va, va + sz);
	}

	while (sz != 0) {
		pmap_enter(pmap_kernel(), va, pa, prot, flags | PMAP_WIRED);
		va += PAGE_SIZE;
		pa += PAGE_SIZE;
		sz -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
}

int
efi_rt_enter(void)
{
	if (RT == NULL)
		return ENXIO;
	
	mutex_enter(&efi_lock);
	x86_disable_intr();
	//lcr3((register_t) efi_pm->pm_pdirpa);
	fpu_kern_enter();
	return 0;
}

void
efi_rt_exit(void)
{
	mutex_exit(&efi_lock);
	fpu_kern_leave();
	x86_enable_intr();
}

efi_status
efi_rt_gettime(struct efi_tm *tm, struct efi_tmcap *tmcap)
{
	efi_status status;

	if (RT == NULL || RT->rt_gettime == NULL)
		return ENXIO;

	efi_rt_enter();
	aprint_normal("efirt entered\n");
	status = RT->rt_gettime(tm, tmcap);
	efi_rt_exit();
	aprint_normal("efirt exited\n");

	return status;
}

efi_status
efi_rt_settime(struct efi_tm *tm)
{
	efi_status status;

	if (RT == NULL || RT->rt_settime == NULL)
		return ENXIO;

	efi_rt_enter();
	status = RT->rt_settime(tm);
	efi_rt_exit();

	return status;
}

efi_status
efi_rt_getvar(uint16_t *name, struct uuid *vendor, uint32_t *attrib, u_long *datasize, void *data)
{
	efi_status status;

	if (RT == NULL || RT->rt_getvar == NULL) {
		return ENXIO;
	}

	efi_rt_enter();
	status = RT->rt_getvar(name, vendor, attrib, datasize, data);
	efi_rt_exit();

	return status;
}

efi_status
efi_rt_nextvar(u_long *namesize, efi_char *name, struct uuid *vendor)
{
	efi_status status;

	if (RT == NULL || RT->rt_scanvar == NULL) {
		return ENXIO;
	}

	efi_rt_enter();
	status = RT->rt_scanvar(namesize, name, vendor);
	efi_rt_exit();

	return status;
}

efi_status
efi_rt_setvar(uint16_t *name, struct uuid *vendor, uint32_t attrib, u_long datasize, void *data)
{
	efi_status status;

	if (RT == NULL || RT->rt_scanvar == NULL) {
		return ENXIO;
	}

	efi_rt_enter();
	status = RT->rt_setvar(name, vendor, attrib, datasize, data);
	efi_rt_exit();

	return status;
}

efi_status
efi_rt_reset(enum efi_reset type)
{
	efi_status status;

	if (RT == NULL || RT->rt_reset == NULL)
		return ENXIO;

	efi_rt_enter();
	status = RT->rt_reset(type, 0, 0, NULL);
	efi_rt_exit();

	return status;
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
