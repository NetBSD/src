/*	$NetBSD: apei_bert.c,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $	*/

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

/*
 * APEI BERT -- Boot Error Record Table
 *
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#boot-error-source
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_bert.c,v 1.1.4.2 2024/10/09 13:00:11 martin Exp $");

#include <sys/types.h>

#include <sys/systm.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apeivar.h>
#include <dev/acpi/apei_bertvar.h>

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("apei")

/*
 * apei_bert_attach(sc)
 *
 *	Scan the Boot Error Record Table for hardware errors that
 *	happened early at boot or on the previous boot.
 */
void
apei_bert_attach(struct apei_softc *sc)
{
	const ACPI_TABLE_BERT *bert = sc->sc_tab.bert;
	struct apei_bert_softc *bsc = &sc->sc_bert;
	bool fatal = false;

	/*
	 * Verify the table is large enough.
	 */
	if (bert->Header.Length < sizeof(*bert)) {
		aprint_error_dev(sc->sc_dev, "BERT: truncated table:"
		    " %"PRIu32" < %zu bytes\n",
		    bert->Header.Length, sizeof(*bert));
		return;
	}

	/*
	 * In verbose boots, print the BERT physical address and
	 * length.  The operator might find this handy for dd'ing it
	 * from /dev/mem, if allowed.
	 */
	aprint_verbose_dev(sc->sc_dev, "BERT: 0x%x bytes at 0x%"PRIx64"\n",
	    bert->RegionLength, bert->Address);

	/*
	 * Verify the length is enough for a Generic Error Status Block
	 * header, at least.
	 */
	if (bert->RegionLength < sizeof(*bsc->bsc_gesb)) {
		aprint_error_dev(sc->sc_dev,
		    "BERT: truncated boot error region, %"PRIu32" < %zu bytes",
		    bert->RegionLength, sizeof(*bsc->bsc_gesb));
		return;
	}

	/*
	 * Map the GESB and process it, but don't acknowledge it --
	 * this is a one-time polled source; it won't (or at least,
	 * shouldn't) change after boot.
	 */
	bsc->bsc_gesb = AcpiOsMapMemory(bert->Address, bert->RegionLength);
	const uint32_t status = apei_gesb_report(sc, bsc->bsc_gesb,
	    bert->RegionLength, "boot error record", &fatal);
	if (status == 0) {
		/*
		 * If there were no boot errors, leave a note in dmesg
		 * to this effect without cluttering up the console
		 * unless you asked for it by `boot -v'.
		 */
		aprint_verbose_dev(sc->sc_dev,
		    "BERT: no boot errors recorded\n");
	}

	/*
	 * If the error was fatal, print a warning to the console.
	 * Probably not actually fatal now since it is usually related
	 * to early or previous boot.
	 */
	if (fatal) {
		aprint_error_dev(sc->sc_dev, "BERT:"
		    " fatal pre-boot error recorded\n");
	}

	/* XXX expose content via sysctl? */
}

/*
 * apei_bert_detach(sc)
 *
 *	Free any software resources associated with the Boot Error
 *	Record Table.
 */
void
apei_bert_detach(struct apei_softc *sc)
{
	const ACPI_TABLE_BERT *bert = sc->sc_tab.bert;
	struct apei_bert_softc *bsc = &sc->sc_bert;

	if (bsc->bsc_gesb) {
		AcpiOsUnmapMemory(bsc->bsc_gesb, bert->RegionLength);
		bsc->bsc_gesb = NULL;
	}
}
