/* $NetBSD: efi.c,v 1.3 2022/04/01 06:51:12 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: efi.c,v 1.3 2022/04/01 06:51:12 skrll Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/efiio.h>

#include <dev/efivar.h>

#ifdef _LP64
#define	EFIERR(x)		(0x8000000000000000 | x)
#else
#define	EFIERR(x)		(0x80000000 | x)
#endif

#define	EFI_SUCCESS		0
#define	EFI_INVALID_PARAMETER	EFIERR(2)
#define	EFI_UNSUPPORTED		EFIERR(3)
#define	EFI_BUFFER_TOO_SMALL	EFIERR(5)
#define	EFI_DEVICE_ERROR	EFIERR(7)
#define	EFI_WRITE_PROTECTED	EFIERR(8)
#define	EFI_OUT_OF_RESOURCES	EFIERR(9)
#define	EFI_NOT_FOUND		EFIERR(14)
#define	EFI_SECURITY_VIOLATION	EFIERR(26)

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
static u_int efi_isopen = 0;

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
	if (atomic_cas_uint(&efi_isopen, 0, 1) == 1) {
		return EBUSY;
	}
	return 0;
}

static int
efi_close(dev_t dev, int flags, int type, struct lwp *l)
{
	KASSERT(efi_isopen);
	atomic_swap_uint(&efi_isopen, 0);
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

static int
efi_ioctl_var_get(struct efi_var_ioc *var)
{
	uint16_t *namebuf;
	void *databuf = NULL;
	size_t datasize;
	efi_status status;
	int error;

	if (var->name == NULL || var->namesize == 0 ||
	    (var->data != NULL && var->datasize == 0)) {
		return EINVAL;
	}
	if (var->namesize > EFI_VARNAME_MAXLENGTH) {
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
	datasize = var->datasize;
	if (datasize != 0) {
		databuf = kmem_alloc(datasize, KM_SLEEP);
		error = copyin(var->data, databuf, datasize);
		if (error != 0) {
			goto done;
		}
	}

	status = efi_ops->efi_getvar(namebuf, &var->vendor, &var->attrib,
	    &var->datasize, databuf);
	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL) {
		error = efi_status_to_error(status);
		goto done;
	}
	if (status == EFI_SUCCESS && databuf != NULL) {
		error = copyout(databuf, var->data, var->datasize);
	} else {
		var->data = NULL;
	}

done:
	kmem_free(namebuf, var->namesize);
	if (databuf != NULL) {
		kmem_free(databuf, datasize);
	}
	return error;
}

static int
efi_ioctl_var_next(struct efi_var_ioc *var)
{
	efi_status status;
	uint16_t *namebuf;
	size_t namesize;
	int error;

	if (var->name == NULL || var->namesize == 0) {
		return EINVAL;
	}
	if (var->namesize > EFI_VARNAME_MAXLENGTH) {
		return ENOMEM;
	}

	namesize = var->namesize;
	namebuf = kmem_alloc(namesize, KM_SLEEP);
	error = copyin(var->name, namebuf, namesize);
	if (error != 0) {
		goto done;
	}

	status = efi_ops->efi_nextvar(&var->namesize, namebuf, &var->vendor);
	if (status != EFI_SUCCESS && status != EFI_BUFFER_TOO_SMALL) {
		error = efi_status_to_error(status);
		goto done;
	}
	if (status == EFI_SUCCESS) {
		error = copyout(namebuf, var->name, var->namesize);
	} else {
		var->name = NULL;
	}

done:
	kmem_free(namebuf, namesize);
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
