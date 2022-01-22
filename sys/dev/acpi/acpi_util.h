/*	$NetBSD: acpi_util.h,v 1.14 2022/01/22 11:49:17 thorpej Exp $ */

/*-
 * Copyright (c) 2003, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
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

/*
 * Copyright 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_DEV_ACPI_ACPI_UTIL_H
#define _SYS_DEV_ACPI_ACPI_UTIL_H

devhandle_t	devhandle_from_acpi(devhandle_t, ACPI_HANDLE);
ACPI_HANDLE	devhandle_to_acpi(devhandle_t);

#define	ACPI_DEVICE_CALL_REGISTER(_n_, _c_)				\
	DEVICE_CALL_REGISTER(acpi_device_calls, _n_, _c_)

ACPI_STATUS	acpi_eval_integer(ACPI_HANDLE, const char *, ACPI_INTEGER *);
ACPI_STATUS	acpi_eval_set_integer(ACPI_HANDLE handle, const char *path,
		    ACPI_INTEGER arg);
ACPI_STATUS	acpi_eval_string(ACPI_HANDLE, const char *, char **);
ACPI_STATUS	acpi_eval_struct(ACPI_HANDLE, const char *, ACPI_BUFFER *);
ACPI_STATUS	acpi_eval_reference_handle(ACPI_OBJECT *, ACPI_HANDLE *);

ACPI_STATUS	acpi_foreach_package_object(ACPI_OBJECT *,
			ACPI_STATUS (*)(ACPI_OBJECT *, void *), void *);
ACPI_STATUS	acpi_get(ACPI_HANDLE, ACPI_BUFFER *,
			ACPI_STATUS (*)(ACPI_HANDLE, ACPI_BUFFER *));

struct acpi_devnode *acpi_match_node(ACPI_HANDLE handle);
void		     acpi_match_node_init(struct acpi_devnode *ad);

const char	*acpi_name(ACPI_HANDLE);
int		 acpi_match_hid(ACPI_DEVICE_INFO *, const char * const *);
int		 acpi_match_class(ACPI_HANDLE, uint8_t, uint8_t, uint8_t);
ACPI_HANDLE	 acpi_match_cpu_info(struct cpu_info *);
struct cpu_info *acpi_match_cpu_handle(ACPI_HANDLE);

char		*acpi_pack_compat_list(ACPI_DEVICE_INFO *, size_t *);

ACPI_STATUS	 acpi_dsd_integer(ACPI_HANDLE, const char *, ACPI_INTEGER *);
ACPI_STATUS	 acpi_dsd_string(ACPI_HANDLE, const char *, char **);
ACPI_STATUS	 acpi_dsd_bool(ACPI_HANDLE, const char *, bool *);

ACPI_STATUS	 acpi_dsm(ACPI_HANDLE, uint8_t *, ACPI_INTEGER,
			ACPI_INTEGER, const ACPI_OBJECT *, ACPI_OBJECT **);
ACPI_STATUS	 acpi_dsm_typed(ACPI_HANDLE, uint8_t *, ACPI_INTEGER,
			ACPI_INTEGER, const ACPI_OBJECT *,
			ACPI_OBJECT_TYPE, ACPI_OBJECT **);
ACPI_STATUS	 acpi_dsm_integer(ACPI_HANDLE, uint8_t *, ACPI_INTEGER,
			ACPI_INTEGER, const ACPI_OBJECT *,
			ACPI_INTEGER *);
ACPI_STATUS	 acpi_dsm_query(ACPI_HANDLE, uint8_t *, ACPI_INTEGER,
			ACPI_INTEGER *);

ACPI_STATUS	 acpi_claim_childdevs(device_t, struct acpi_devnode *);

#endif	/* !_SYS_DEV_ACPI_ACPI_UTIL_H */
