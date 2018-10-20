/* $NetBSD: acpi_machdep.h,v 1.1.2.2 2018/10/20 06:58:25 pgoyette Exp $ */

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

#ifndef _ARM_ACPI_MACHDEP_H
#define _ARM_ACPI_MACHDEP_H

struct acpi_softc;

ACPI_STATUS		acpi_md_OsInitialize(void);
ACPI_PHYSICAL_ADDRESS	acpi_md_OsGetRootPointer(void);
ACPI_STATUS		acpi_md_OsInstallInterruptHandler(UINT32, ACPI_OSD_HANDLER,
							  void *, void **, const char *);
void			acpi_md_OsRemoveInterruptHandler(void *);
ACPI_STATUS		acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS, UINT32, void **);
void			acpi_md_OsUnmapMemory(void *, UINT32);
ACPI_STATUS		acpi_md_OsGetPhysicalAddress(void *, ACPI_PHYSICAL_ADDRESS *);
BOOLEAN			acpi_md_OsReadable(void *, UINT32);
BOOLEAN			acpi_md_OsWritable(void *, UINT32);
void			acpi_md_OsEnableInterrupt(void);
void			acpi_md_OsDisableInterrupt(void);
int			acpi_md_sleep(int);
uint32_t		acpi_md_pdc(void);
uint32_t		acpi_md_ncpus(void);
void			acpi_md_callback(struct acpi_softc *);

static inline int
_acpi_notimpl(const char *fn)
{
	printf("WARNING: ACPI function %s not implemented on this platform\n", fn);
	return 0;
}

#define	acpi_md_OsIn8(x)	_acpi_notimpl("acpi_md_OsIn8")
#define	acpi_md_OsIn16(x)	_acpi_notimpl("acpi_md_OsIn16")
#define	acpi_md_OsIn32(x)	_acpi_notimpl("acpi_md_OsIn32")
#define	acpi_md_OsOut8(x, v)	(void)_acpi_notimpl("acpi_md_OsOut8")
#define	acpi_md_OsOut16(x, v)	(void)_acpi_notimpl("acpi_md_OsOut16")
#define	acpi_md_OsOut32(x, v)	(void)_acpi_notimpl("acpi_md_OsOut32")

#endif /* !_ARM_ACPI_MACHDEP_H */
