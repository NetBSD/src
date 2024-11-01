/*	$NetBSD: apei_hest.c,v 1.3.4.3 2024/11/01 14:45:36 martin Exp $	*/

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
 * APEI HEST -- Hardware Error Source Table
 *
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#acpi-error-source
 *
 * XXX uncorrectable error NMI comes in on all CPUs at once, what to do?
 *
 * XXX AMD MCA
 *
 * XXX IA32 machine check stuff
 *
 * XXX switch-to-polling for GHES notifications
 *
 * XXX error threshold for GHES notifications
 *
 * XXX sort out interrupt notification types, e.g. do we ever need to
 * do acpi_intr_establish?
 *
 * XXX sysctl knob to force polling each particular error source that
 * supports it
 *
 * XXX consider a lighter-weight polling schedule for machines with
 * thousands of polled GHESes
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_hest.c,v 1.3.4.3 2024/11/01 14:45:36 martin Exp $");

#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/systm.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_cper.h>
#include <dev/acpi/apei_hestvar.h>
#include <dev/acpi/apei_hed.h>
#include <dev/acpi/apei_mapreg.h>
#include <dev/acpi/apeivar.h>

#if defined(__i386__) || defined(__x86_64__)
#include <x86/nmi.h>
#endif

#include "ioconf.h"

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("apei")

/*
 * apei_hest_ghes_handle(sc, src)
 *
 *	Check for, report, and acknowledge any error from a Generic
 *	Hardware Error Source (GHES, not GHESv2).  Return true if there
 *	was any error to report, false if not.
 */
static bool
apei_hest_ghes_handle(struct apei_softc *sc, struct apei_source *src)
{
	ACPI_HEST_GENERIC *ghes = container_of(src->as_header,
	    ACPI_HEST_GENERIC, Header);
	ACPI_HEST_GENERIC_STATUS *gesb = src->as_ghes.gesb;
	char ctx[sizeof("error source 65535")];
	uint32_t status;
	bool fatal = false;

	/*
	 * Process and report any error.
	 */
	snprintf(ctx, sizeof(ctx), "error source %"PRIu16,
	    ghes->Header.SourceId);
	status = apei_gesb_report(sc, src->as_ghes.gesb,
	    ghes->ErrorBlockLength, ctx, &fatal);

	/*
	 * Acknowledge the error by clearing the block status.  To
	 * avoid races, we probably have to avoid further access to the
	 * GESB until we get another notification.
	 *
	 * As a precaution, we zero this with atomic compare-and-swap
	 * so at least we can see if the status changed while we were
	 * working on it.
	 *
	 * It is tempting to clear bits with atomic and-complement, but
	 * the BlockStatus is not just a bit mask -- bits [13:4] are a
	 * count of Generic Error Data Entries, and who knows what bits
	 * [31:14] might be used for in the future.
	 *
	 * XXX The GHES(v1) protocol is unclear from the specification
	 * here.  The GHESv2 protocol has a separate register write to
	 * acknowledge, which is a bit clearer.
	 */
	membar_release();
	const uint32_t status1 = atomic_cas_32(&gesb->BlockStatus, status, 0);
	if (status1 != status) {
		device_printf(sc->sc_dev, "%s: status changed from"
		    " 0x%"PRIx32" to 0x%"PRIx32"\n",
		    ctx, status, status1);
	}

	/*
	 * If the error was fatal, panic now.
	 */
	if (fatal)
		panic("fatal hardware error");

	return status != 0;
}

/*
 * apei_hest_ghes_v2_handle(sc, src)
 *
 *	Check for, report, and acknowledge any error from a Generic
 *	Hardware Error Source v2.  Return true if there was any error
 *	to report, false if not.
 */
static bool
apei_hest_ghes_v2_handle(struct apei_softc *sc, struct apei_source *src)
{
	ACPI_HEST_GENERIC_V2 *ghes_v2 = container_of(src->as_header,
	    ACPI_HEST_GENERIC_V2, Header);
	ACPI_HEST_GENERIC_STATUS *gesb = src->as_ghes.gesb;
	char ctx[sizeof("error source 65535")];
	uint64_t X;
	uint32_t status;
	bool fatal;

	/*
	 * Process and report any error.
	 */
	snprintf(ctx, sizeof(ctx), "error source %"PRIu16,
	    ghes_v2->Header.SourceId);
	status = apei_gesb_report(sc, src->as_ghes.gesb,
	    ghes_v2->ErrorBlockLength, ctx, &fatal);

	/*
	 * First clear the block status.  As a precaution, we zero this
	 * with atomic compare-and-swap so at least we can see if the
	 * status changed while we were working on it.
	 */
	membar_release();
	const uint32_t status1 = atomic_cas_32(&gesb->BlockStatus, status, 0);
	if (status1 != status) {
		device_printf(sc->sc_dev, "%s: status changed from"
		    " 0x%"PRIx32" to 0x%"PRIx32"\n",
		    ctx, status, status1);
	}

	/*
	 * Next, do the Read Ack dance.
	 *
	 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#generic-hardware-error-source-version-2-ghesv2-type-10
	 */
	X = apei_mapreg_read(&ghes_v2->ReadAckRegister,
	    src->as_ghes_v2.read_ack);
	X &= ghes_v2->ReadAckPreserve;
	X |= ghes_v2->ReadAckWrite;
	apei_mapreg_write(&ghes_v2->ReadAckRegister,
	    src->as_ghes_v2.read_ack, X);

	/*
	 * If the error was fatal, panic now.
	 */
	if (fatal)
		panic("fatal hardware error");

	return status != 0;
}

/*
 * apei_hest_ghes_poll(cookie)
 *
 *	Callout handler for periodic polling of a Generic Hardware
 *	Error Source (GHES, not GHESv2), using Notification Type `0 -
 *	Polled'.
 *
 *	cookie is the struct apei_source pointer for a single source;
 *	if there are multiple sources there will be multiple callouts.
 */
static void
apei_hest_ghes_poll(void *cookie)
{
	struct apei_source *src = cookie;
	struct apei_softc *sc = src->as_sc;
	ACPI_HEST_GENERIC *ghes = container_of(src->as_header,
	    ACPI_HEST_GENERIC, Header);

	/*
	 * Process and acknowledge any error.
	 */
	(void)apei_hest_ghes_handle(sc, src);

	/*
	 * Schedule polling again after the firmware-suggested
	 * interval.
	 */
	callout_schedule(&src->as_ch,
	    MAX(1, mstohz(ghes->Notify.PollInterval)));
}

/*
 * apei_hest_ghes_v2_poll(cookie)
 *
 *	Callout handler for periodic polling of a Generic Hardware
 *	Error Source v2, using Notification Type `0 - Polled'.
 *
 *	cookie is the struct apei_source pointer for a single source;
 *	if there are multiple sources there will be multiple callouts.
 */
static void
apei_hest_ghes_v2_poll(void *cookie)
{
	struct apei_source *src = cookie;
	struct apei_softc *sc = src->as_sc;
	ACPI_HEST_GENERIC_V2 *ghes_v2 = container_of(src->as_header,
	    ACPI_HEST_GENERIC_V2, Header);

	/*
	 * Process and acknowledge any error.
	 */
	(void)apei_hest_ghes_v2_handle(sc, src);

	/*
	 * Schedule polling again after the firmware-suggested
	 * interval.
	 */
	callout_schedule(&src->as_ch,
	    MAX(1, mstohz(ghes_v2->Notify.PollInterval)));
}

#if defined(__i386__) || defined(__x86_64__)

/*
 * The NMI is (sometimes?) delivered to all CPUs at once.  To reduce
 * confusion, let's try to have only one CPU process error
 * notifications at a time.
 */
static __cpu_simple_lock_t apei_hest_nmi_lock = __SIMPLELOCK_UNLOCKED;

/*
 * apei_hest_ghes_nmi(tf, cookie)
 *
 *	Nonmaskable interrupt handler for Generic Hardware Error
 *	Sources (GHES, not GHESv2) with Notification Type `4 - NMI'.
 */
static int
apei_hest_ghes_nmi(const struct trapframe *tf, void *cookie)
{
	struct apei_source *src = cookie;
	struct apei_softc *sc = src->as_sc;

	__cpu_simple_lock(&apei_hest_nmi_lock);
	const bool mine = apei_hest_ghes_handle(sc, src);
	__cpu_simple_unlock(&apei_hest_nmi_lock);

	/*
	 * Tell the NMI subsystem whether this interrupt could have
	 * been for us or not.
	 */
	return mine;
}

/*
 * apei_hest_ghes_v2_nmi(tf, cookie)
 *
 *	Nonmaskable interrupt handler for Generic Hardware Error
 *	Sources v2 with Notification Type `4 - NMI'.
 */
static int
apei_hest_ghes_v2_nmi(const struct trapframe *tf, void *cookie)
{
	struct apei_source *src = cookie;
	struct apei_softc *sc = src->as_sc;

	__cpu_simple_lock(&apei_hest_nmi_lock);
	const bool mine = apei_hest_ghes_v2_handle(sc, src);
	__cpu_simple_unlock(&apei_hest_nmi_lock);

	/*
	 * Tell the NMI subsystem whether this interrupt could have
	 * been for us or not.
	 */
	return mine;
}

#endif	/* defined(__i386__) || defined(__x86_64__) */

/*
 * apei_hest_attach_ghes(sc, ghes, i)
 *
 *	Attach a Generic Hardware Error Source (GHES, not GHESv2) as
 *	the ith source in the Hardware Error Source Table.
 *
 *	After this point, the system will check for and handle errors
 *	when notified by this source.
 */
static void
apei_hest_attach_ghes(struct apei_softc *sc, ACPI_HEST_GENERIC *ghes,
    uint32_t i)
{
	struct apei_hest_softc *hsc = &sc->sc_hest;
	struct apei_source *src = &hsc->hsc_source[i];
	uint64_t addr;
	ACPI_STATUS rv;
	char ctx[sizeof("HEST[4294967295, Id=65535]")];

	snprintf(ctx, sizeof(ctx), "HEST[%"PRIu32", Id=%"PRIu16"]",
	    i, ghes->Header.SourceId);

	/*
	 * Verify the source is enabled before proceeding.  The Enabled
	 * field is 8 bits with 256 possibilities, but only two of the
	 * possibilities, 0 and 1, have semantics defined in the spec,
	 * so out of an abundance of caution let's tread carefully in
	 * case anything changes and noisily reject any values other
	 * than 1.
	 */
	switch (ghes->Enabled) {
	case 1:
		break;
	case 0:
		aprint_debug_dev(sc->sc_dev, "%s: disabled\n", ctx);
		return;
	default:
		aprint_error_dev(sc->sc_dev, "%s: unknown GHES Enabled state:"
		    " 0x%"PRIx8"\n", ctx, ghes->Enabled);
		return;
	}

	/*
	 * Verify the Error Status Address bit width is at most 64 bits
	 * before proceeding with this source.  When we get 128-bit
	 * addressing, this code will have to be updated.
	 */
	if (ghes->ErrorStatusAddress.BitWidth > 64) {
		aprint_error_dev(sc->sc_dev, "%s: excessive address bits:"
		    " %"PRIu8"\n", ctx, ghes->ErrorStatusAddress.BitWidth);
		return;
	}

	/*
	 * Read the GHES Error Status Addresss.  This is the physical
	 * address of a GESB, Generic Error Status Block.  Why the
	 * physical address is exposed via this indirection, and not
	 * simply stored directly in the GHES, is unclear to me.
	 * Hoping it's not because the address can change dynamically,
	 * because the error handling path shouldn't involve mapping
	 * anything.
	 */
	rv = AcpiRead(&addr, &ghes->ErrorStatusAddress);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "%s:"
		    " failed to read error status address: %s", ctx,
		    AcpiFormatException(rv));
		return;
	}
	aprint_debug_dev(sc->sc_dev, "%s: error status @ 0x%"PRIx64"\n", ctx,
	    addr);

	/*
	 * Initialize the source and map the GESB so we can get at it
	 * in the error handling path.
	 */
	src->as_sc = sc;
	src->as_header = &ghes->Header;
	src->as_ghes.gesb = AcpiOsMapMemory(addr, ghes->ErrorBlockLength);

	/*
	 * Arrange to receive notifications.
	 */
	switch (ghes->Notify.Type) {
	case ACPI_HEST_NOTIFY_POLLED:
		if (ghes->Notify.PollInterval == 0) /* paranoia */
			break;
		callout_init(&src->as_ch, CALLOUT_MPSAFE);
		callout_setfunc(&src->as_ch, &apei_hest_ghes_poll, src);
		callout_schedule(&src->as_ch, 0);
		break;
	case ACPI_HEST_NOTIFY_SCI:
	case ACPI_HEST_NOTIFY_GPIO:
		/*
		 * SCI and GPIO notifications are delivered through
		 * Hardware Error Device (PNP0C33) events.
		 *
		 * XXX Where is this spelled out?  The text at
		 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#event-notification-for-generic-error-sources
		 * is vague.
		 */
		SIMPLEQ_INSERT_TAIL(&hsc->hsc_hed_list, src, as_entry);
		break;
#if defined(__i386__) || defined(__x86_64__)
	case ACPI_HEST_NOTIFY_NMI:
		src->as_nmi = nmi_establish(&apei_hest_ghes_nmi, src);
		break;
#endif
	}

	/*
	 * Now that we have notification set up, process and
	 * acknowledge the initial GESB report if any.
	 */
	apei_hest_ghes_handle(sc, src);
}

/*
 * apei_hest_detach_ghes(sc, ghes, i)
 *
 *	Detach the ith source, which is a Generic Hardware Error Source
 *	(GHES, not GHESv2).
 *
 *	After this point, the system will ignore notifications from
 *	this source.
 */
static void
apei_hest_detach_ghes(struct apei_softc *sc, ACPI_HEST_GENERIC *ghes,
    uint32_t i)
{
	struct apei_hest_softc *hsc = &sc->sc_hest;
	struct apei_source *src = &hsc->hsc_source[i];

	/*
	 * Arrange to stop receiving notifications.
	 */
	switch (ghes->Notify.Type) {
	case ACPI_HEST_NOTIFY_POLLED:
		if (ghes->Notify.PollInterval == 0) /* paranoia */
			break;
		callout_halt(&src->as_ch, NULL);
		callout_destroy(&src->as_ch);
		break;
	case ACPI_HEST_NOTIFY_SCI:
	case ACPI_HEST_NOTIFY_GPIO:
		/*
		 * No need to spend time removing the entry; no further
		 * calls via apei_hed_notify are possible at this
		 * point, now that detach has begun.
		 */
		break;
#if defined(__i386__) || defined(__x86_64__)
	case ACPI_HEST_NOTIFY_NMI:
		nmi_disestablish(src->as_nmi);
		src->as_nmi = NULL;
		break;
#endif
	}

	/*
	 * No more notifications.  Unmap the GESB and destroy the
	 * interrupt source now that it will no longer be used in
	 * error handling path.
	 */
	AcpiOsUnmapMemory(src->as_ghes.gesb, ghes->ErrorBlockLength);
	src->as_ghes.gesb = NULL;
	src->as_header = NULL;
	src->as_sc = NULL;
}


/*
 * apei_hest_attach_ghes_v2(sc, ghes_v2, i)
 *
 *	Attach a Generic Hardware Error Source v2 as the ith source in
 *	the Hardware Error Source Table.
 *
 *	After this point, the system will check for and handle errors
 *	when notified by this source.
 */
static void
apei_hest_attach_ghes_v2(struct apei_softc *sc, ACPI_HEST_GENERIC_V2 *ghes_v2,
    uint32_t i)
{
	struct apei_hest_softc *hsc = &sc->sc_hest;
	struct apei_source *src = &hsc->hsc_source[i];
	uint64_t addr;
	struct apei_mapreg *read_ack;
	ACPI_STATUS rv;
	char ctx[sizeof("HEST[4294967295, Id=65535]")];

	snprintf(ctx, sizeof(ctx), "HEST[%"PRIu32", Id=%"PRIu16"]",
	    i, ghes_v2->Header.SourceId);

	/*
	 * Verify the source is enabled before proceeding.  The Enabled
	 * field is 8 bits with 256 possibilities, but only two of the
	 * possibilities, 0 and 1, have semantics defined in the spec,
	 * so out of an abundance of caution let's tread carefully in
	 * case anything changes and noisily reject any values other
	 * than 1.
	 */
	switch (ghes_v2->Enabled) {
	case 1:
		break;
	case 0:
		aprint_debug_dev(sc->sc_dev, "%s: disabled\n", ctx);
		return;
	default:
		aprint_error_dev(sc->sc_dev, "%s:"
		    " unknown GHESv2 Enabled state: 0x%"PRIx8"\n", ctx,
		    ghes_v2->Enabled);
		return;
	}

	/*
	 * Verify the Error Status Address bit width is at most 64 bits
	 * before proceeding with this source.  When we get 128-bit
	 * addressing, this code will have to be updated.
	 */
	if (ghes_v2->ErrorStatusAddress.BitWidth > 64) {
		aprint_error_dev(sc->sc_dev, "%s: excessive address bits:"
		    " %"PRIu8"\n", ctx, ghes_v2->ErrorStatusAddress.BitWidth);
		return;
	}

	/*
	 * Read the GHESv2 Error Status Addresss.  This is the physical
	 * address of a GESB, Generic Error Status Block.  Why the
	 * physical address is exposed via this indirection, and not
	 * simply stored directly in the GHESv2, is unclear to me.
	 * Hoping it's not because the address can change dynamically,
	 * because the error handling path shouldn't involve mapping
	 * anything.
	 */
	rv = AcpiRead(&addr, &ghes_v2->ErrorStatusAddress);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "%s:"
		    " failed to read error status address: %s", ctx,
		    AcpiFormatException(rv));
		return;
	}
	aprint_debug_dev(sc->sc_dev, "%s: error status @ 0x%"PRIx64"\n", ctx,
	    addr);

	/*
	 * Try to map the Read Ack register up front, so we don't have
	 * to allocate and free kva in AcpiRead/AcpiWrite at the time
	 * we're handling an error.  Bail if we can't.
	 */
	read_ack = apei_mapreg_map(&ghes_v2->ReadAckRegister);
	if (read_ack == NULL) {
		aprint_error_dev(sc->sc_dev, "%s:"
		    " unable to map Read Ack register\n", ctx);
		return;
	}

	/*
	 * Initialize the source and map the GESB it in the error
	 * handling path.
	 */
	src->as_sc = sc;
	src->as_header = &ghes_v2->Header;
	src->as_ghes_v2.gesb = AcpiOsMapMemory(addr,
	    ghes_v2->ErrorBlockLength);
	src->as_ghes_v2.read_ack = read_ack;

	/*
	 * Arrange to receive notifications.
	 */
	switch (ghes_v2->Notify.Type) {
	case ACPI_HEST_NOTIFY_POLLED:
		if (ghes_v2->Notify.PollInterval == 0) /* paranoia */
			break;
		callout_init(&src->as_ch, CALLOUT_MPSAFE);
		callout_setfunc(&src->as_ch, &apei_hest_ghes_v2_poll, src);
		callout_schedule(&src->as_ch, 0);
		break;
	case ACPI_HEST_NOTIFY_SCI:
	case ACPI_HEST_NOTIFY_GPIO:
		/*
		 * SCI and GPIO notifications are delivered through
		 * Hardware Error Device (PNP0C33) events.
		 *
		 * XXX Where is this spelled out?  The text at
		 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#event-notification-for-generic-error-sources
		 * is vague.
		 */
		SIMPLEQ_INSERT_TAIL(&hsc->hsc_hed_list, src, as_entry);
		break;
#if defined(__i386__) || defined(__x86_64__)
	case ACPI_HEST_NOTIFY_NMI:
		src->as_nmi = nmi_establish(&apei_hest_ghes_v2_nmi, src);
		break;
#endif
	}

	/*
	 * Now that we have notification set up, process and
	 * acknowledge the initial GESB report if any.
	 */
	apei_hest_ghes_handle(sc, src);
}

/*
 * apei_hest_detach_ghes_v2(sc, ghes_v2, i)
 *
 *	Detach the ith source, which is a Generic Hardware Error Source
 *	v2.
 *
 *	After this point, the system will ignore notifications from
 *	this source.
 */
static void
apei_hest_detach_ghes_v2(struct apei_softc *sc, ACPI_HEST_GENERIC_V2 *ghes_v2,
    uint32_t i)
{
	struct apei_hest_softc *hsc = &sc->sc_hest;
	struct apei_source *src = &hsc->hsc_source[i];

	/*
	 * Arrange to stop receiving notifications.
	 */
	switch (ghes_v2->Notify.Type) {
	case ACPI_HEST_NOTIFY_POLLED:
		if (ghes_v2->Notify.PollInterval == 0) /* paranoia */
			break;
		callout_halt(&src->as_ch, NULL);
		callout_destroy(&src->as_ch);
		break;
	case ACPI_HEST_NOTIFY_SCI:
	case ACPI_HEST_NOTIFY_GPIO:
		/*
		 * No need to spend time removing the entry; no further
		 * calls via apei_hed_notify are possible at this
		 * point, now that detach has begun.
		 */
		break;
#if defined(__i386__) || defined(__x86_64__)
	case ACPI_HEST_NOTIFY_NMI:
		nmi_disestablish(src->as_nmi);
		src->as_nmi = NULL;
		break;
#endif
	}

	/*
	 * No more notifications.  Unmap the GESB and read ack register
	 * now that it will no longer be used in error handling path.
	 */
	AcpiOsUnmapMemory(src->as_ghes_v2.gesb, ghes_v2->ErrorBlockLength);
	src->as_ghes_v2.gesb = NULL;
	apei_mapreg_unmap(&ghes_v2->ReadAckRegister, src->as_ghes_v2.read_ack);
	src->as_ghes_v2.read_ack = NULL;
	src->as_header = NULL;
	src->as_sc = NULL;
}

/*
 * apei_hest_attach_source(sc, header, i, size_t maxlen)
 *
 *	Attach the ith source in the Hardware Error Source Table given
 *	its header, and return a pointer to the header of the next
 *	source in the table, provided it is no more than maxlen bytes
 *	past header.  Return NULL if the size of the source is unknown
 *	or would exceed maxlen bytes.
 */
static ACPI_HEST_HEADER *
apei_hest_attach_source(struct apei_softc *sc, ACPI_HEST_HEADER *header,
    uint32_t i, size_t maxlen)
{
	char ctx[sizeof("HEST[4294967295, Id=65535]")];

	snprintf(ctx, sizeof(ctx), "HEST[%"PRIu32", Id=%"PRIu16"]",
	    i, header->SourceId);

	switch (header->Type) {
	case ACPI_HEST_TYPE_IA32_CHECK: {
		ACPI_HEST_IA_MACHINE_CHECK *const imc = container_of(header,
		    ACPI_HEST_IA_MACHINE_CHECK, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*imc))
			return NULL;
		maxlen -= sizeof(*imc);
		ACPI_HEST_IA_ERROR_BANK *const bank = (void *)(imc + 1);
		if (maxlen < imc->NumHardwareBanks*sizeof(*bank))
			return NULL;
		return (ACPI_HEST_HEADER *)(bank + imc->NumHardwareBanks);
	}
	case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK: {
		ACPI_HEST_IA_CORRECTED *const imcc = container_of(header,
		    ACPI_HEST_IA_CORRECTED, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*imcc))
			return NULL;
		maxlen -= sizeof(*imcc);
		ACPI_HEST_IA_ERROR_BANK *const bank = (void *)(imcc + 1);
		if (maxlen < imcc->NumHardwareBanks*sizeof(*bank))
			return NULL;
		return (ACPI_HEST_HEADER *)(bank + imcc->NumHardwareBanks);
	}
	case ACPI_HEST_TYPE_IA32_NMI: {
		ACPI_HEST_IA_NMI *const ianmi = container_of(header,
		    ACPI_HEST_IA_NMI, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*ianmi))
			return NULL;
		return (ACPI_HEST_HEADER *)(ianmi + 1);
	}
	case ACPI_HEST_TYPE_AER_ROOT_PORT: {
		ACPI_HEST_AER_ROOT *const aerroot = container_of(header,
		    ACPI_HEST_AER_ROOT, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*aerroot))
			return NULL;
		return (ACPI_HEST_HEADER *)(aerroot + 1);
	}
	case ACPI_HEST_TYPE_AER_ENDPOINT: {
		ACPI_HEST_AER *const aer = container_of(header,
		    ACPI_HEST_AER, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*aer))
			return NULL;
		return (ACPI_HEST_HEADER *)(aer + 1);
	}
	case ACPI_HEST_TYPE_AER_BRIDGE: {
		ACPI_HEST_AER_BRIDGE *const aerbridge = container_of(header,
		    ACPI_HEST_AER_BRIDGE, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*aerbridge))
			return NULL;
		return (ACPI_HEST_HEADER *)(aerbridge + 1);
	}
	case ACPI_HEST_TYPE_GENERIC_ERROR: {
		ACPI_HEST_GENERIC *const ghes = container_of(header,
		    ACPI_HEST_GENERIC, Header);

		if (maxlen < sizeof(*ghes))
			return NULL;
		apei_hest_attach_ghes(sc, ghes, i);
		return (ACPI_HEST_HEADER *)(ghes + 1);
	}
	case ACPI_HEST_TYPE_GENERIC_ERROR_V2: {
		ACPI_HEST_GENERIC_V2 *const ghes_v2 = container_of(header,
		    ACPI_HEST_GENERIC_V2, Header);

		if (maxlen < sizeof(*ghes_v2))
			return NULL;
		apei_hest_attach_ghes_v2(sc, ghes_v2, i);
		return (ACPI_HEST_HEADER *)(ghes_v2 + 1);
	}
	case ACPI_HEST_TYPE_IA32_DEFERRED_CHECK: {
		ACPI_HEST_IA_DEFERRED_CHECK *const imdc = container_of(header,
		    ACPI_HEST_IA_DEFERRED_CHECK, Header);

		aprint_error_dev(sc->sc_dev, "%s:"
		    " unimplemented type: 0x%04"PRIx16"\n", ctx, header->Type);

		if (maxlen < sizeof(*imdc))
			return NULL;
		maxlen -= sizeof(*imdc);
		ACPI_HEST_IA_ERROR_BANK *const bank = (void *)(imdc + 1);
		if (maxlen < imdc->NumHardwareBanks*sizeof(*bank))
			return NULL;
		return (ACPI_HEST_HEADER *)(bank + imdc->NumHardwareBanks);
	}
	case ACPI_HEST_TYPE_NOT_USED3:
	case ACPI_HEST_TYPE_NOT_USED4:
	case ACPI_HEST_TYPE_NOT_USED5:
	default:
		aprint_error_dev(sc->sc_dev, "%s: unknown type:"
		    " 0x%04"PRIx16"\n", ctx, header->Type);
		if (header->Type >= 12) {
			/*
			 * `Beginning with error source type 12 and
			 *  onward, each Error Source Structure must
			 *  use the standard Error Source Structure
			 *  Header as defined below.'
			 *
			 * Not yet in acpica, though, so we copy this
			 * down manually.
			 */
			struct {
				UINT16	Type;
				UINT16	Length;
			} *const essh = (void *)header;

			if (maxlen < sizeof(*essh) || maxlen < essh->Length)
				return NULL;
			return (ACPI_HEST_HEADER *)((char *)header +
			    essh->Length);
		}
		return NULL;
	}
}

/*
 * apei_hest_detach_source(sc, header, i)
 *
 *	Detach the ith source in the Hardware Error Status Table.
 *	Caller is assumed to have stored where each source's header is,
 *	so no need to return the pointer to the header of the next
 *	source in the table.
 */
static void
apei_hest_detach_source(struct apei_softc *sc, ACPI_HEST_HEADER *header,
    uint32_t i)
{

	switch (header->Type) {
	case ACPI_HEST_TYPE_GENERIC_ERROR: {
		ACPI_HEST_GENERIC *ghes = container_of(header,
		    ACPI_HEST_GENERIC, Header);

		apei_hest_detach_ghes(sc, ghes, i);
		break;
	}
	case ACPI_HEST_TYPE_GENERIC_ERROR_V2: {
		ACPI_HEST_GENERIC_V2 *ghes_v2 = container_of(header,
		    ACPI_HEST_GENERIC_V2, Header);

		apei_hest_detach_ghes_v2(sc, ghes_v2, i);
		break;
	}
	case ACPI_HEST_TYPE_IA32_CHECK:
	case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
	case ACPI_HEST_TYPE_IA32_NMI:
	case ACPI_HEST_TYPE_NOT_USED3:
	case ACPI_HEST_TYPE_NOT_USED4:
	case ACPI_HEST_TYPE_NOT_USED5:
	case ACPI_HEST_TYPE_AER_ROOT_PORT:
	case ACPI_HEST_TYPE_AER_ENDPOINT:
	case ACPI_HEST_TYPE_AER_BRIDGE:
	case ACPI_HEST_TYPE_IA32_DEFERRED_CHECK:
	default:
		/* XXX shouldn't happen */
		break;
	}
}

/*
 * apei_hest_attach(sc)
 *
 *	Scan the Hardware Error Source Table and attach sources
 *	enumerated in it so we can receive and process hardware errors
 *	during operation.
 */
void
apei_hest_attach(struct apei_softc *sc)
{
	ACPI_TABLE_HEST *hest = sc->sc_tab.hest;
	struct apei_hest_softc *hsc = &sc->sc_hest;
	ACPI_HEST_HEADER *header, *next;
	uint32_t i, n;
	size_t resid;

	/*
	 * Initialize the HED (Hardware Error Device, PNP0C33)
	 * notification list so apei_hed_notify becomes a noop with no
	 * extra effort even if we fail to attach anything.
	 */
	SIMPLEQ_INIT(&hsc->hsc_hed_list);

	/*
	 * Verify the table is large enough.
	 */
	if (hest->Header.Length < sizeof(*hest)) {
		aprint_error_dev(sc->sc_dev, "HEST: truncated table:"
		    " %"PRIu32" < %zu minimum bytes\n",
		    hest->Header.Length, sizeof(*hest));
		return;
	}

	n = hest->ErrorSourceCount;
	aprint_normal_dev(sc->sc_dev, "HEST: %"PRIu32
	    " hardware error source%s\n", n, n == 1 ? "" : "s");

	/*
	 * This could be SIZE_MAX but let's put a smaller arbitrary
	 * limit on it; if you have gigabytes of HEST something is
	 * probably wrong.
	 */
	if (n > MIN(SIZE_MAX, INT32_MAX)/sizeof(hsc->hsc_source[0])) {
		aprint_error_dev(sc->sc_dev, "HEST: too many error sources\n");
		return;
	}
	hsc->hsc_source = kmem_zalloc(n * sizeof(hsc->hsc_source[0]),
	    KM_SLEEP);

	header = (ACPI_HEST_HEADER *)(hest + 1);
	resid = hest->Header.Length - sizeof(*hest);
	for (i = 0; i < n && resid; i++, header = next) {
		next = apei_hest_attach_source(sc, header, i, resid);
		if (next == NULL) {
			aprint_error_dev(sc->sc_dev, "truncated source:"
			    " %"PRIu32"\n", i);
			break;
		}
		KASSERT(header < next);
		KASSERT((size_t)((const char *)next - (const char *)header) <=
		    resid);
		resid -= (const char *)next - (const char *)header;
	}
	if (resid) {
		aprint_error_dev(sc->sc_dev, "HEST:"
		    " %zu bytes of trailing garbage after %"PRIu32" entries\n",
		    resid, n);
	}
}

/*
 * apei_hest_detach(sc)
 *
 *	Stop receiving and processing hardware error notifications and
 *	free resources set up from the Hardware Error Source Table.
 */
void
apei_hest_detach(struct apei_softc *sc)
{
	ACPI_TABLE_HEST *hest = sc->sc_tab.hest;
	struct apei_hest_softc *hsc = &sc->sc_hest;
	uint32_t i, n;

	if (hsc->hsc_source) {
		n = hest->ErrorSourceCount;
		for (i = 0; i < n; i++) {
			struct apei_source *src = &hsc->hsc_source[i];
			ACPI_HEST_HEADER *header = src->as_header;

			if (src->as_header == NULL)
				continue;
			apei_hest_detach_source(sc, header, i);
		}
		kmem_free(hsc->hsc_source, n * sizeof(hsc->hsc_source[0]));
		hsc->hsc_source = NULL;
	}
}

void
apei_hed_notify(void)
{
	device_t apei0;
	struct apei_softc *sc;
	struct apei_hest_softc *hsc;
	struct apei_source *src;

	/*
	 * Take a reference to the apei0 device so it doesn't go away
	 * while we're working.
	 */
	if ((apei0 = device_lookup_acquire(&apei_cd, 0)) == NULL)
		goto out;
	sc = device_private(apei0);

	/*
	 * If there's no HEST, nothing to do.
	 */
	if (sc->sc_tab.hest == NULL)
		goto out;
	hsc = &sc->sc_hest;

	/*
	 * Walk through the HED-notified hardware error sources and
	 * check them.  The list is stable until we release apei0.
	 */
	SIMPLEQ_FOREACH(src, &hsc->hsc_hed_list, as_entry) {
		ACPI_HEST_HEADER *const header = src->as_header;

		switch (header->Type) {
		case ACPI_HEST_TYPE_GENERIC_ERROR:
			apei_hest_ghes_handle(sc, src);
			break;
		case ACPI_HEST_TYPE_GENERIC_ERROR_V2:
			apei_hest_ghes_v2_handle(sc, src);
			break;
		case ACPI_HEST_TYPE_IA32_CHECK:
		case ACPI_HEST_TYPE_IA32_CORRECTED_CHECK:
		case ACPI_HEST_TYPE_IA32_NMI:
		case ACPI_HEST_TYPE_NOT_USED3:
		case ACPI_HEST_TYPE_NOT_USED4:
		case ACPI_HEST_TYPE_NOT_USED5:
		case ACPI_HEST_TYPE_AER_ROOT_PORT:
		case ACPI_HEST_TYPE_AER_ENDPOINT:
		case ACPI_HEST_TYPE_AER_BRIDGE:
//		case ACPI_HEST_TYPE_GENERIC_ERROR:
//		case ACPI_HEST_TYPE_GENERIC_ERROR_V2:
		case ACPI_HEST_TYPE_IA32_DEFERRED_CHECK:
		default:
			/* XXX shouldn't happen */
			break;
		}
	}

out:	if (apei0) {
		device_release(apei0);
		apei0 = NULL;
	}
}
