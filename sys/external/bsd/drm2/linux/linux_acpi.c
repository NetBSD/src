/*	$NetBSD: linux_acpi.c,v 1.1 2022/02/27 14:22:21 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_acpi.c,v 1.1 2022/02/27 14:22:21 riastradh Exp $");

#include <linux/acpi.h>

union acpi_object *
acpi_evaluate_dsm(acpi_handle handle, const guid_t *uuid, u64 rev, u64 func,
    union acpi_object *argv4)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT params[4];
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	arg.Count = 4;
	arg.Pointer = params;
	params[0].Type = ACPI_TYPE_BUFFER;
	params[0].Buffer.Length = 16;
	params[0].Buffer.Pointer = (char *)__UNCONST(uuid);
	params[1].Type = ACPI_TYPE_INTEGER;
	params[1].Integer.Value = rev;
	params[2].Type = ACPI_TYPE_INTEGER;
	params[2].Integer.Value = func;
	if (argv4 != NULL) {
		params[3] = *argv4;
	} else {
		params[3].Type = ACPI_TYPE_PACKAGE;
		params[3].Package.Count = 0;
		params[3].Package.Elements = NULL;
	}

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObject(handle, "_DSM", &arg, &buf);
	if (ACPI_SUCCESS(rv))
		return (ACPI_OBJECT *)buf.Pointer;
	return NULL;
}

union acpi_object *
acpi_evaluate_dsm_typed(acpi_handle handle, const guid_t *uuid, u64 rev,
    u64 func, union acpi_object *argv4, acpi_object_type type)
{
	union acpi_object *obj;

	obj = acpi_evaluate_dsm(handle, uuid, rev, func, argv4);
	if (obj != NULL && obj->Type != type) {
		ACPI_FREE(obj);
		obj = NULL;
	}
	return obj;
}

#define	ACPI_INIT_DSM_ARGV4(cnt, eles)		\
{						\
	.Package.Type = ACPI_TYPE_PACKAGE,	\
	.Package.Count = (cnt),			\
	.Package.Elements = (eles)		\
}

bool
acpi_check_dsm(acpi_handle handle, const guid_t *uuid, u64 rev, u64 funcs)
{
	ACPI_OBJECT *obj;
	uint64_t mask = 0;
	int i;

	if (funcs == 0)
		return false;

	obj = acpi_evaluate_dsm(handle, uuid, rev, 0, NULL);
	if (obj == NULL)
		return false;

	if (obj->Type == ACPI_TYPE_INTEGER)
		mask = obj->Integer.Value;
	else if (obj->Type == ACPI_TYPE_BUFFER)
		for (i = 0; i < obj->Buffer.Length && i < 8; i++)
			mask |= (uint64_t)obj->Buffer.Pointer[i] << (i * 8);
	ACPI_FREE(obj);

	if ((mask & 0x1) == 0x1 && (mask & funcs) == funcs)
		return true;
	return false;
}
