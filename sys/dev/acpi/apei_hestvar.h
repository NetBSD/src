/*	$NetBSD: apei_hestvar.h,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $	*/

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

#ifndef	_SYS_DEV_ACPI_APEI_HESTVAR_H_
#define	_SYS_DEV_ACPI_APEI_HESTVAR_H_

#include <sys/queue.h>
#include <sys/callout.h>

#include <dev/acpi/acpivar.h>

struct apei_mapreg;
struct apei_softc;

#if defined(__i386__) || defined(__x86_64__)
struct nmi_handler;
#endif

/*
 * struct apei_source
 *
 *	Software state for a hardware error source from the HEST,
 *	Hardware Error Source Table, to process error notifications.
 */
struct apei_source {
	struct apei_softc		*as_sc;
	ACPI_HEST_HEADER		*as_header;
	union {
		struct {
			ACPI_HEST_GENERIC_STATUS	*gesb;
		}			as_ghes;
		struct {
			ACPI_HEST_GENERIC_STATUS	*gesb;
			struct apei_mapreg		*read_ack;
		}			as_ghes_v2;
	};
	union {
		struct callout			as_ch;
#if defined(__i386__) || defined(__x86_64__)
		struct nmi_handler		*as_nmi;
#endif
		SIMPLEQ_ENTRY(apei_source)	as_entry;
	};
};

/*
 * struct apei_hest_softc
 *
 *	Software state for processing hardware error reports during
 *	operation, from the HEST, Hardware Error Source table.
 */
struct apei_hest_softc {
	struct apei_source		*hsc_source;
	SIMPLEQ_HEAD(, apei_source)	hsc_hed_list;
};

void apei_hest_attach(struct apei_softc *);
void apei_hest_detach(struct apei_softc *);

#endif	/* _SYS_DEV_ACPI_APEI_HESTVAR_H_ */
