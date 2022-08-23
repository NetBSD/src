/* $NetBSD: efi_runtime.c,v 1.9 2022/06/18 08:13:44 skrll Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#include "efi.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efi_runtime.c,v 1.9 2022/06/18 08:13:44 skrll Exp $");

#include <sys/param.h>
#include <sys/mutex.h>
#include <sys/endian.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/efivar.h>

#include <arm/arm/efi_runtime.h>

#ifdef _LP64
#define	EFIERR(x)	(0x8000000000000000 | x)
#else
#define	EFIERR(x)	(0x80000000 | x)
#endif

#define	EFI_UNSUPPORTED		EFIERR(3)
#define	EFI_DEVICE_ERROR	EFIERR(7)

#define EFI_TABLE_ALLOC_MAX 0x800000

static kmutex_t efi_lock;
static struct efi_rt *RT;
static struct efi_systbl *efi_systbl;
static struct efi_cfgtbl *efi_cfgtbl;

#if BYTE_ORDER == LITTLE_ENDIAN
static struct efi_rt efi_rtcopy;

#if NEFI > 0
static struct efi_ops arm_efi_ops = {
	.efi_gettable	= arm_efirt_gettable,
	.efi_copytable	= arm_efirt_copytable,
	.efi_gettime	= arm_efirt_gettime,
	.efi_settime	= arm_efirt_settime,
	.efi_getvar		= arm_efirt_getvar,
	.efi_setvar		= arm_efirt_setvar,
	.efi_nextvar	= arm_efirt_nextvar,
};
#endif
#endif

int
arm_efirt_init(paddr_t efi_system_table)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	struct efi_systbl *ST;
	const size_t sz = PAGE_SIZE * 2;
	vaddr_t va, cva;
	paddr_t cpa;

	va = uvm_km_alloc(kernel_map, sz, 0, UVM_KMF_VAONLY);
	if (va == 0) {
		aprint_error("%s: can't allocate VA\n", __func__);
		return ENOMEM;
	}
	for (cva = va, cpa = trunc_page(efi_system_table);
	     cva < va + sz;
	     cva += PAGE_SIZE, cpa += PAGE_SIZE) {
		pmap_kenter_pa(cva, cpa, VM_PROT_READ, 0);
	}
	pmap_update(pmap_kernel());

	ST = (void *)(va + (efi_system_table - trunc_page(efi_system_table)));
	if (ST->st_hdr.th_sig != EFI_SYSTBL_SIG) {
		aprint_error("EFI: signature mismatch (%#" PRIx64 " != %#"
		    PRIx64 ")\n", ST->st_hdr.th_sig, EFI_SYSTBL_SIG);
		return EINVAL;
	}

	efi_systbl = ST;
	efi_cfgtbl = ST->st_cfgtbl;

	struct efi_rt *rt = ST->st_rt;
	mutex_init(&efi_lock, MUTEX_DEFAULT, IPL_HIGH);

	pmap_activate_efirt();

	memcpy(&efi_rtcopy, rt, sizeof(efi_rtcopy));
	RT = &efi_rtcopy;

	pmap_deactivate_efirt();

#if NEFI > 0
	efi_register_ops(&arm_efi_ops);
#endif

	/*
	 *	DEBUG
	 */


	aprint_normal("efi runtime services: online\n");


	/*
	 *	END sDEBUG
	 */

	return 0;
#else
	/* EFI runtime not supported in big endian mode */
	return ENXIO;
#endif
}

efi_status
arm_efirt_gettable(struct uuid *uuid, void **ptr)
{
	struct efi_cfgtbl *ct;
	u_long count;
	int error;

	if (efi_cfgtbl == NULL || efi_systbl == NULL)
		return ENXIO;

	aprint_normal("DEBUG efirt_gettable 1\n");

	mutex_enter(&efi_lock);
	error = arm_efirt_md_enter();
	if (error != 0)
		return error;

	aprint_normal("DEBUG efirt_gettable 2\n");

	count = efi_systbl->st_entries;

	aprint_normal("DEBUG efirt_gettable 3: st entries count: %ld\n", count);

	ct = efi_cfgtbl;
	while (count--) {
		uuid_printf(&ct->ct_uuid);
		printf("\n");

		if (!bcmp(&ct->ct_uuid, uuid, sizeof(*uuid))) {
			*ptr = ct->ct_data;

			mutex_exit(&efi_lock);
			arm_efirt_md_exit();
			return 0;
		}
		ct++;
	}

	aprint_normal("DEBUG efirt_gettable 3\n");

	mutex_exit(&efi_lock);
	arm_efirt_md_exit();
	return ENOENT;
}

efi_status
arm_efirt_copytable(struct uuid *uuid, void **buf, size_t buf_len, size_t *table_len)
{
	static const struct known_table {
		struct uuid uuid;
		enum efi_table_type type;
	} tables[] = {
		{ EFI_TABLE_ESRT,       TYPE_ESRT },
		{ EFI_PROPERTIES_TABLE, TYPE_PROP }
	};
	size_t table_idx;
	void *taddr;
	int rc;

	aprint_normal("DEBUG efirt_copytable 1\n");

	for (table_idx = 0; table_idx < NUM_OF_TYPES; table_idx++) {
		if (!bcmp(&tables[table_idx].uuid, uuid, sizeof(*uuid)))
			break;
	}

	aprint_normal("DEBUG efirt_copytable 2\n");

	if (table_idx == NUM_OF_TYPES) {
		aprint_normal("DEBUG efirt_copytable - couldnt find esrt table\n");
		return EINVAL;
	}

	rc = arm_efirt_gettablelength(tables[table_idx].type, table_len, &taddr);

	aprint_normal("DEBUG efirt_copytable 3\n");

	if (rc != 0)
		return rc;

	aprint_normal("DEBUG efirt_copytable 4\n");

	/* return table length to userspace */
	if (buf == NULL)
		return 0;

	aprint_normal("DEBUG efirt_copytable 5\n");

	*buf = kmem_alloc(*table_len, KM_SLEEP);
	rc = copyout(taddr, *buf, *table_len);
	return rc;
}

int
arm_efirt_gettablelength(enum efi_table_type type, size_t *table_len, void **taddr)
{
	switch (type) {
		case TYPE_ESRT:
		{
			struct efi_esrt_table *esrt = NULL;
			struct uuid uuid = EFI_TABLE_ESRT;
			uint32_t fw_resource_count = 0;
			size_t len = sizeof(*esrt);
			int error;
			void *buf;

			aprint_normal("DEBUG efirt_gettablelength 1\n");

			error = arm_efirt_gettable(&uuid, (void **)&esrt);

			aprint_normal("DEBUG efirt_gettablelength: get table call error: %d\n", error);

			if (error != 0)
				return error;

			aprint_normal("DEBUG efirt_gettablelength 2\n");

			buf = kmem_alloc(len, KM_SLEEP);
			error = copystr(esrt, buf, len, &len);
			if (error != 0) {
				kmem_free(buf, len);
				return (error);
			}

			/* Check ESRT version */
			if (((struct efi_esrt_table *)buf)->fw_resource_version !=
		    	ESRT_FIRMWARE_RESOURCE_VERSION) {
				kmem_free(buf, len);
				return ENODEV;
			}

			aprint_normal("DEBUG efirt_gettablelength 3\n");

			fw_resource_count = ((struct efi_esrt_table *)buf)->
		    	fw_resource_count;
			if (fw_resource_count > EFI_TABLE_ALLOC_MAX /
		    	sizeof(struct efi_esrt_entry_v1)) {
				kmem_free(buf, len);
				return ENOMEM;
			}

			aprint_normal("DEBUG efirt_gettablelength 4\n");

			len += fw_resource_count * sizeof(struct efi_esrt_entry_v1);
			*table_len = len;

			if (taddr != NULL)
				*taddr = esrt;
			kmem_free(buf, len);
			return 0;
		}
		case TYPE_PROP:
		{
			return ENOTSUP;
		}
		case NUM_OF_TYPES:
			return ENOENT;
	}
	return ENOENT;
}

efi_status
arm_efirt_gettime(struct efi_tm *tm, struct efi_tmcap *tmcap)
{
	efi_status status = EFI_DEVICE_ERROR;

	if (RT == NULL || RT->rt_gettime == NULL) {
		return EFI_UNSUPPORTED;
	}

	mutex_enter(&efi_lock);
	if (arm_efirt_md_enter() == 0) {
		status = RT->rt_gettime(tm, tmcap);
	}
	arm_efirt_md_exit();
	mutex_exit(&efi_lock);

	return status;
}

efi_status
arm_efirt_settime(struct efi_tm *tm)
{
	efi_status status = EFI_DEVICE_ERROR;

	if (RT == NULL || RT->rt_settime == NULL) {
		return EFI_UNSUPPORTED;
	}

	mutex_enter(&efi_lock);
	if (arm_efirt_md_enter() == 0) {
		status = RT->rt_settime(tm);
	}
	arm_efirt_md_exit();
	mutex_exit(&efi_lock);

	return status;
}

efi_status
arm_efirt_getvar(uint16_t *name, struct uuid *vendor, uint32_t *attrib,
    u_long *datasize, void *data)
{
	efi_status status = EFI_DEVICE_ERROR;

	if (RT == NULL || RT->rt_getvar == NULL) {
		return EFI_UNSUPPORTED;
	}

	mutex_enter(&efi_lock);
	if (arm_efirt_md_enter() == 0) {
		status = RT->rt_getvar(name, vendor, attrib, datasize, data);
	}
	arm_efirt_md_exit();
	mutex_exit(&efi_lock);

	return status;
}

efi_status
arm_efirt_nextvar(u_long *namesize, efi_char *name, struct uuid *vendor)
{
	efi_status status = EFI_DEVICE_ERROR;

	if (RT == NULL || RT->rt_scanvar == NULL) {
		return EFI_UNSUPPORTED;
	}

	mutex_enter(&efi_lock);
	if (arm_efirt_md_enter() == 0) {
		status = RT->rt_scanvar(namesize, name, vendor);
	}
	arm_efirt_md_exit();
	mutex_exit(&efi_lock);

	return status;
}

efi_status
arm_efirt_setvar(uint16_t *name, struct uuid *vendor, uint32_t attrib,
    u_long datasize, void *data)
{
	efi_status status = EFI_DEVICE_ERROR;

	if (RT == NULL || RT->rt_setvar == NULL) {
		return EFI_UNSUPPORTED;
	}

	mutex_enter(&efi_lock);
	if (arm_efirt_md_enter() == 0) {
		status = RT->rt_setvar(name, vendor, attrib, datasize, data);
	}
	arm_efirt_md_exit();
	mutex_exit(&efi_lock);

	return status;
}

int
arm_efirt_reset(enum efi_reset type)
{
	static int reset_called = false;
	int error;

	if (RT == NULL || RT->rt_reset == NULL)
		return ENXIO;

	mutex_enter(&efi_lock);
	if (reset_called == false) {
		reset_called = true;
		if ((error = arm_efirt_md_enter()) == 0) {
			if (RT->rt_reset(type, 0, 0, NULL) != 0) {
				error = EIO;
			}
		}
		arm_efirt_md_exit();
	} else {
		error = EPERM;
	}
	mutex_exit(&efi_lock);

	return error;
}
