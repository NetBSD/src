/* $NetBSD: efi.c,v 1.9 2023/05/24 00:02:51 riastradh Exp $ */

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This pseudo-driver implements a /dev/efi character device that provides
 * ioctls for using UEFI runtime time and variable services.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efi.c,v 1.9 2023/05/24 00:02:51 riastradh Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/efiio.h>

#include <uvm/uvm_extern.h>

#include <dev/efivar.h>
#include <dev/mm.h>

#include "ioconf.h"

/*
 * Maximum length of an EFI variable name. The UEFI spec doesn't specify a
 * constraint, but we want to limit the size to act as a guard rail against
 * allocating too much kernel memory.
 */
#define	EFI_VARNAME_MAXLENGTH		EFI_PAGE_SIZE

/*
 * Pointer to arch specific EFI backend.
 */
static const struct efi_ops *efi_ops = NULL;

/*
 * Only allow one user of /dev/efi at a time. Even though the MD EFI backends
 * should serialize individual UEFI RT calls, the UEFI specification says
 * that a SetVariable() call between calls to GetNextVariableName() may
 * produce unpredictable results, and we want to avoid this.
 */
static volatile u_int efi_isopen = 0;

static dev_type_open(efi_open);
static dev_type_close(efi_close);
static dev_type_ioctl(efi_ioctl);

const struct cdevsw efi_cdevsw = {
	.d_open =	efi_open,
	.d_close =	efi_close,
	.d_ioctl =	efi_ioctl,
	.d_read =	noread,
	.d_write =	nowrite,
	.d_stop =	nostop,
	.d_tty =	notty,
	.d_poll =	nopoll,
	.d_mmap =	nommap,
	.d_kqfilter =	nokqfilter,
	.d_discard =	nodiscard,
	.d_flag =	D_OTHER | D_MPSAFE,
};

static int
efi_open(dev_t dev, int flags, int type, struct lwp *l)
{

	if (efi_ops == NULL) {
		return ENXIO;
	}
	if (atomic_swap_uint(&efi_isopen, 1) == 1) {
		return EBUSY;
	}
	membar_acquire();
	return 0;
}

static int
efi_close(dev_t dev, int flags, int type, struct lwp *l)
{

	KASSERT(efi_isopen);
	atomic_store_release(&efi_isopen, 0);
	return 0;
}

static int
efi_status_to_error(efi_status status)
{
	switch (status) {
	case EFI_SUCCESS:
		return 0;
	case EFI_INVALID_PARAMETER:
		return EINVAL;
	case EFI_UNSUPPORTED:
		return EOPNOTSUPP;
	case EFI_BUFFER_TOO_SMALL:
		return ERANGE;
	case EFI_DEVICE_ERROR:
		return EIO;
	case EFI_WRITE_PROTECTED:
		return EROFS;
	case EFI_OUT_OF_RESOURCES:
		return ENOMEM;
	case EFI_NOT_FOUND:
		return ENOENT;
	case EFI_SECURITY_VIOLATION:
		return EACCES;
	default:
		return EIO;
	}
}

/* XXX move to efi.h */
#define	EFI_SYSTEM_RESOURCE_TABLE_GUID					      \
	{0xb122a263,0x3661,0x4f68,0x99,0x29,{0x78,0xf8,0xb0,0xd6,0x21,0x80}}
#define	EFI_PROPERTIES_TABLE						      \
	{0x880aaca3,0x4adc,0x4a04,0x90,0x79,{0xb7,0x47,0x34,0x08,0x25,0xe5}}

#define	EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION	1

struct EFI_SYSTEM_RESOURCE_ENTRY {
	struct uuid	FwClass;
	uint32_t	FwType;
	uint32_t	FwVersion;
	uint32_t	LowestSupportedFwVersion;
	uint32_t	CapsuleFlags;
	uint32_t	LastAttemptVersion;
	uint32_t	LastAttemptStatus;
};

struct EFI_SYSTEM_RESOURCE_TABLE {
	uint32_t	FwResourceCount;
	uint32_t	FwResourceCountMax;
	uint64_t	FwResourceVersion;
	struct EFI_SYSTEM_RESOURCE_ENTRY	Entries[];
};

static void *
efi_map_pa(uint64_t addr, bool *directp)
{
	paddr_t pa = addr;
	vaddr_t va;

	/*
	 * Verify the address is not truncated by conversion to
	 * paddr_t.  This might happen with a 64-bit EFI booting a
	 * 32-bit OS.
	 */
	if (pa != addr)
		return NULL;

	/*
	 * Try direct-map if we have it.  If it works, note that it was
	 * direct-mapped for efi_unmap.
	 */
#ifdef __HAVE_MM_MD_DIRECT_MAPPED_PHYS
	if (mm_md_direct_mapped_phys(pa, &va)) {
		*directp = true;
		return (void *)va;
	}
#endif

	/*
	 * No direct map.  Reserve a page of kernel virtual address
	 * space, with no backing, to map to the physical address.
	 */
	va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY|UVM_KMF_WAITVA);
	KASSERT(va != 0);

	/*
	 * Map the kva page to the physical address and update the
	 * kernel pmap so we can use it.
	 */
	pmap_kenter_pa(va, pa, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	/*
	 * Success!  Return the VA and note that it was not
	 * direct-mapped for efi_unmap.
	 */
	*directp = false;
	return (void *)va;
}

static void
efi_unmap(void *ptr, bool direct)
{
	vaddr_t va = (vaddr_t)ptr;

	/*
	 * If it was direct-mapped, nothing to do here.
	 */
	if (direct)
		return;

	/*
	 * First remove the mapping from the kernel pmap so that it can
	 * be reused, before we free the kva and let anyone else reuse
	 * it.
	 */
	pmap_kremove(va, PAGE_SIZE);
	pmap_update(pmap_kernel());

	/*
	 * Next free the kva so it can be reused by someone else.
	 */
	uvm_km_free(kernel_map, va, PAGE_SIZE, UVM_KMF_VAONLY);
}

static int
efi_ioctl_got_table(struct efi_get_table_ioc *ioc, void *ptr, size_t len)
{

	/*
	 * Return the actual table length.
	 */
	ioc->table_len = len;

	/*
	 * Copy out as much as we can into the user's allocated buffer.
	 */
	return copyout(ptr, ioc->buf, MIN(ioc->buf_len, len));
}

static int
efi_ioctl_get_esrt(struct efi_get_table_ioc *ioc,
    struct EFI_SYSTEM_RESOURCE_TABLE *tab)
{

	/*
	 * Verify the firmware resource version is one we understand.
	 */
	if (tab->FwResourceVersion !=
	    EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION)
		return ENOENT;

	/*
	 * Verify the resource count fits within the single page we
	 * have mapped.
	 *
	 * XXX What happens if it doesn't?  Are we expected to map more
	 * than one page, according to the table header?  The UEFI spec
	 * is unclear on this.
	 */
	const size_t entry_space = PAGE_SIZE -
	    offsetof(struct EFI_SYSTEM_RESOURCE_TABLE, Entries);
	if (tab->FwResourceCount > entry_space/sizeof(tab->Entries[0]))
		return ENOENT;

	/*
	 * Success!  Return everything through the last table entry.
	 */
	const size_t len = offsetof(struct EFI_SYSTEM_RESOURCE_TABLE,
	    Entries[tab->FwResourceCount]);
	return efi_ioctl_got_table(ioc, tab, len);
}

static int
efi_ioctl_get_table(struct efi_get_table_ioc *ioc)
{
	uint64_t addr;
	bool direct;
	efi_status status;
	int error;

	/*
	 * If the platform doesn't support it yet, fail now.
	 */
	if (efi_ops->efi_gettab == NULL)
		return ENODEV;

	/*
	 * Get the address of the requested table out of the EFI
	 * configuration table.
	 */
	status = efi_ops->efi_gettab(&ioc->uuid, &addr);
	if (status != EFI_SUCCESS)
		return efi_status_to_error(status);

	/*
	 * UEFI provides no generic way to identify the size of the
	 * table, so we have to bake knowledge of every vendor GUID
	 * into this code to safely expose the right amount of data to
	 * userland.
	 *
	 * We even have to bake knowledge of which ones are physically
	 * addressed and which ones might be virtually addressed
	 * according to the vendor GUID into this code, although for
	 * the moment we never use RT->SetVirtualAddressMap so we only
	 * ever have to deal with physical addressing.
	 */
	if (memcmp(&ioc->uuid, &(struct uuid)EFI_SYSTEM_RESOURCE_TABLE_GUID,
		sizeof(ioc->uuid)) == 0) {
		struct EFI_SYSTEM_RESOURCE_TABLE *tab;

		if ((tab = efi_map_pa(addr, &direct)) == NULL)
			return ENOENT;
		error = efi_ioctl_get_esrt(ioc, tab);
		efi_unmap(tab, direct);
	} else {
		error = ENOENT;
	}

	return error;
}

static int
efi_ioctl_var_get(struct efi_var_ioc *var)
{
	uint16_t *namebuf;
	void *databuf = NULL;
	size_t databufsize;
	unsigned long datasize;
	efi_status status;
	int error;

	if (var->name == NULL || var->namesize == 0 ||
	    (var->data != NULL && var->datasize == 0)) {
		return EINVAL;
	}
	if (var->namesize > EFI_VARNAME_MAXLENGTH) {
		return ENOMEM;
	}
	if (var->datasize > ULONG_MAX) { /* XXX stricter limit */
		return ENOMEM;
	}

	namebuf = kmem_alloc(var->namesize, KM_SLEEP);
	error = copyin(var->name, namebuf, var->namesize);
	if (error != 0) {
		goto done;
	}
	if (namebuf[var->namesize / 2 - 1] != '\0') {
		error = EINVAL;
		goto done;
	}
	databufsize = var->datasize;
	if (databufsize != 0) {
		databuf = kmem_alloc(databufsize, KM_SLEEP);
		error = copyin(var->data, databuf, databufsize);
		if (error != 0) {
			goto done;
		}
	}

	datasize = databufsize;
	status = efi_ops->efi_getvar(namebuf, &var->vendor, &var->attrib,
	    &datasize, databuf);
	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL) {
		error = efi_status_to_error(status);
		goto done;
	}
	var->datasize = datasize;
	if (status == EFI_SUCCESS && databufsize != 0) {
		error = copyout(databuf, var->data,
		    MIN(datasize, databufsize));
	} else {
		var->data = NULL;
	}

done:
	kmem_free(namebuf, var->namesize);
	if (databuf != NULL) {
		kmem_free(databuf, databufsize);
	}
	return error;
}

static int
efi_ioctl_var_next(struct efi_var_ioc *var)
{
	efi_status status;
	uint16_t *namebuf;
	size_t namebufsize;
	unsigned long namesize;
	int error;

	if (var->name == NULL || var->namesize == 0) {
		return EINVAL;
	}
	if (var->namesize > EFI_VARNAME_MAXLENGTH) {
		return ENOMEM;
	}

	namebufsize = var->namesize;
	namebuf = kmem_alloc(namebufsize, KM_SLEEP);
	error = copyin(var->name, namebuf, namebufsize);
	if (error != 0) {
		goto done;
	}

	CTASSERT(EFI_VARNAME_MAXLENGTH <= ULONG_MAX);
	namesize = namebufsize;
	status = efi_ops->efi_nextvar(&namesize, namebuf, &var->vendor);
	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL) {
		error = efi_status_to_error(status);
		goto done;
	}
	var->namesize = namesize;
	if (status == EFI_SUCCESS) {
		error = copyout(namebuf, var->name,
		    MIN(namesize, namebufsize));
	} else {
		var->name = NULL;
	}

done:
	kmem_free(namebuf, namebufsize);
	return error;
}

static int
efi_ioctl_var_set(struct efi_var_ioc *var)
{
	efi_status status;
	uint16_t *namebuf;
	uint16_t *databuf = NULL;
	int error;

	if (var->name == NULL || var->namesize == 0) {
		return EINVAL;
	}

	namebuf = kmem_alloc(var->namesize, KM_SLEEP);
	error = copyin(var->name, namebuf, var->namesize);
	if (error != 0) {
		goto done;
	}
	if (namebuf[var->namesize / 2 - 1] != '\0') {
		error = EINVAL;
		goto done;
	}
	if (var->datasize != 0) {
		databuf = kmem_alloc(var->datasize, KM_SLEEP);
		error = copyin(var->data, databuf, var->datasize);
		if (error != 0) {
			goto done;
		}
	}

	status = efi_ops->efi_setvar(namebuf, &var->vendor, var->attrib,
	    var->datasize, databuf);
	error = efi_status_to_error(status);

done:
	kmem_free(namebuf, var->namesize);
	if (databuf != NULL) {
		kmem_free(databuf, var->datasize);
	}
	return error;
}

static int
efi_ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	KASSERT(efi_ops != NULL);

	switch (cmd) {
	case EFIIOC_GET_TABLE:
		return efi_ioctl_get_table(data);
	case EFIIOC_VAR_GET:
		return efi_ioctl_var_get(data);
	case EFIIOC_VAR_NEXT:
		return efi_ioctl_var_next(data);
	case EFIIOC_VAR_SET:
		return efi_ioctl_var_set(data);
	}

	return ENOTTY;
}

void
efi_register_ops(const struct efi_ops *ops)
{
	KASSERT(efi_ops == NULL);
	efi_ops = ops;
}

void
efiattach(int count)
{
}
