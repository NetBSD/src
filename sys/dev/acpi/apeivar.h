/*	$NetBSD: apeivar.h,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_DEV_ACPI_APEIVAR_H_
#define	_SYS_DEV_ACPI_APEIVAR_H_

#include <sys/types.h>

#include <sys/device.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_bertvar.h>
#include <dev/acpi/apei_einjvar.h>
#include <dev/acpi/apei_erstvar.h>
#include <dev/acpi/apei_hestvar.h>

struct sysctllog;
struct sysctlnode;

/*
 * struct apei_tab
 *
 *	Collection of pointers to APEI-related ACPI tables.  Used
 *	inside struct apei_softc, and by apei_match without an
 *	apei_softc.
 */
struct apei_tab {
	ACPI_TABLE_BERT	*bert;	/* Boot Error Record Table */
	ACPI_TABLE_EINJ	*einj;	/* Error Injection Table */
	ACPI_TABLE_ERST	*erst;	/* Error Record Serialization Table */
	ACPI_TABLE_HEST	*hest;	/* Hardware Error Source Table */
};

/*
 * struct apei_softc
 *
 *	All software state for APEI.
 */
struct apei_softc {
	device_t		sc_dev;
	struct apei_tab		sc_tab;

	struct sysctllog	*sc_sysctllog;
	const struct sysctlnode	*sc_sysctlroot;

	struct apei_bert_softc	sc_bert;
	struct apei_einj_softc	sc_einj;
	struct apei_erst_softc	sc_erst;
	struct apei_hest_softc	sc_hest;
};

uint32_t apei_gesb_report(struct apei_softc *,
    const ACPI_HEST_GENERIC_STATUS *, size_t, const char *,
    bool *);

#endif	/* _SYS_DEV_ACPI_APEIVAR_H_ */
