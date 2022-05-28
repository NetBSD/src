/*	$NetBSD: acpi.h,v 1.10 2022/05/28 01:07:47 manu Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_ACPI_H_
#define _LINUX_ACPI_H_

#ifdef _KERNEL_OPT
#include "acpica.h"
#endif

#if NACPICA > 0
#include <dev/acpi/acpivar.h>

#include <linux/types.h>
#include <linux/uuid.h>

typedef ACPI_HANDLE acpi_handle;
typedef ACPI_OBJECT_TYPE acpi_object_type;
typedef ACPI_SIZE acpi_size;
typedef ACPI_STATUS acpi_status;

#define	acpi_evaluate_dsm	linux_acpi_evaluate_dsm
#define	acpi_evaluate_dsm_typed	linux_acpi_evaluate_dsm_typed
#define	acpi_check_dsm		linux_acpi_check_dsm

union acpi_object *acpi_evaluate_dsm(acpi_handle, const guid_t *,
    uint64_t, uint64_t, union acpi_object *);
union acpi_object *acpi_evaluate_dsm_typed(acpi_handle, const guid_t *,
    uint64_t, uint64_t, union acpi_object *, acpi_object_type);
bool acpi_check_dsm(acpi_handle, const guid_t *, uint64_t, uint64_t);

#endif	/* NACPICA > 0 */
#endif  /* _LINUX_ACPI_H_ */
