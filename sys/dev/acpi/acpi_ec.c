/*	$NetBSD: acpi_ec.c,v 1.22.2.1 2004/04/28 05:24:33 jmc Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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

/*-
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999, Intel Corp.  All rights
 * reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions 
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.  
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee 
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.  
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE. 
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_ec.c,v 1.22.2.1 2004/04/28 05:24:33 jmc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_ecreg.h>

MALLOC_DECLARE(M_ACPI);

#define _COMPONENT	ACPI_EC_COMPONENT
ACPI_MODULE_NAME("EC")

struct acpi_ec_softc {
	struct device	sc_dev;		/* base device glue */
	ACPI_HANDLE sc_handle;		/* ACPI handle */

	struct acpi_resources sc_res;	/* our bus resources */

	UINT32		sc_gpebit;	/* our GPE interrupt bit */

	bus_space_tag_t	sc_data_st;	/* space tag for data register */
	bus_space_handle_t sc_data_sh;	/* space handle for data register */

	bus_space_tag_t	sc_csr_st;	/* space tag for control register */
	bus_space_handle_t sc_csr_sh;	/* space handle for control register */

	int		sc_flags;	/* see below */

	uint32_t	sc_csrvalue;	/* saved control register */
	uint32_t	sc_uid;		/* _UID in namespace (ECDT only) */

	struct lock	sc_lock;	/* serialize operations to this EC */
	struct simplelock sc_slock;	/* protect against interrupts */
	UINT32		sc_glkhandle;	/* global lock handle */
	UINT32		sc_glk;		/* need global lock? */
};

static const char * const ec_hid[] = {
	"PNP0C09",
	NULL
};

#define	EC_F_TRANSACTION	0x01	/* doing transaction */
#define	EC_F_PENDQUERY		0x02	/* query is pending */

/*
 * how long to wait to acquire global lock.
 * the value is taken from FreeBSD driver.
 */
#define	EC_LOCK_TIMEOUT	1000

#define	EC_DATA_READ(sc)						\
	bus_space_read_1((sc)->sc_data_st, (sc)->sc_data_sh, 0)
#define	EC_DATA_WRITE(sc, v)						\
	bus_space_write_1((sc)->sc_data_st, (sc)->sc_data_sh, 0, (v))

#define	EC_CSR_READ(sc)							\
	bus_space_read_1((sc)->sc_csr_st, (sc)->sc_csr_sh, 0)
#define	EC_CSR_WRITE(sc, v)						\
	bus_space_write_1((sc)->sc_csr_st, (sc)->sc_csr_sh, 0, (v))

typedef struct {
	EC_COMMAND	Command;
	UINT8		Address;
	UINT8		Data;
} EC_REQUEST;

static void		EcGpeHandler(void *Context);
static ACPI_STATUS	EcSpaceSetup(ACPI_HANDLE Region, UINT32 Function,
			    void *Context, void **return_Context);
static ACPI_STATUS	EcSpaceHandler(UINT32 Function,
			    ACPI_PHYSICAL_ADDRESS Address, UINT32 width,
			    ACPI_INTEGER *Value, void *Context,
			    void *RegionContext);

static ACPI_STATUS	EcWaitEvent(struct acpi_ec_softc *sc, EC_EVENT Event);
static ACPI_STATUS	EcQuery(struct acpi_ec_softc *sc, UINT8 *Data);
static ACPI_STATUS	EcTransaction(struct acpi_ec_softc *sc,
			    EC_REQUEST *EcRequest);
static ACPI_STATUS	EcRead(struct acpi_ec_softc *sc, UINT8 Address,
			    UINT8 *Data);
static ACPI_STATUS	EcWrite(struct acpi_ec_softc *sc, UINT8 Address,
			    UINT8 *Data);
static void		EcGpeQueryHandler(void *);
static __inline int	EcIsLocked(struct acpi_ec_softc *);
static __inline void	EcLock(struct acpi_ec_softc *);
static __inline void	EcUnlock(struct acpi_ec_softc *);


int	acpiec_match(struct device *, struct cfdata *, void *);
void	acpiec_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpiec, sizeof(struct acpi_ec_softc),
    acpiec_match, acpiec_attach, NULL, NULL);

static struct acpi_ec_softc *ecdt_sc;

static __inline int
EcIsLocked(struct acpi_ec_softc *sc)
{
 
	return (lockstatus(&sc->sc_lock) == LK_EXCLUSIVE);
}

static __inline void
EcLock(struct acpi_ec_softc *sc)
{
	ACPI_STATUS status;
	int s;

	lockmgr(&sc->sc_lock, LK_EXCLUSIVE, NULL);
	if (sc->sc_glk) {
		status = AcpiAcquireGlobalLock(EC_LOCK_TIMEOUT,
		    &sc->sc_glkhandle);
		if (ACPI_FAILURE(status)) {
			printf("%s: failed to acquire global lock\n",
			    sc->sc_dev.dv_xname);
			lockmgr(&sc->sc_lock, LK_RELEASE, NULL);
			return;
		}
	}

	s = splvm(); /* XXX */
	simple_lock(&sc->sc_slock);
	sc->sc_flags |= EC_F_TRANSACTION;
	simple_unlock(&sc->sc_slock);
	splx(s);
}

static __inline void
EcUnlock(struct acpi_ec_softc *sc)
{
	ACPI_STATUS status;
	int s;

	/*
	 * Clear & Re-Enable the EC GPE:
	 * -----------------------------
	 * 'Consume' any EC GPE events that we generated while performing
	 * the transaction (e.g. IBF/OBF). Clearing the GPE here shouldn't
	 * have an adverse affect on outstanding EC-SCI's, as the source
	 * (EC-SCI) will still be high and thus should trigger the GPE
	 * immediately after we re-enabling it.
	 */
	s = splvm(); /* XXX */
	simple_lock(&sc->sc_slock);
	if (sc->sc_flags & EC_F_PENDQUERY) {
		ACPI_STATUS Status2;

		Status2 = AcpiOsQueueForExecution(OSD_PRIORITY_HIGH,
		    EcGpeQueryHandler, sc);
		if (ACPI_FAILURE(Status2))
			printf("%s: unable to queue pending query: %s\n",
			    sc->sc_dev.dv_xname, AcpiFormatException(Status2));
		sc->sc_flags &= ~EC_F_PENDQUERY;
	}
	sc->sc_flags &= ~EC_F_TRANSACTION;
	simple_unlock(&sc->sc_slock);
	splx(s);

	if (sc->sc_glk) {
		status = AcpiReleaseGlobalLock(sc->sc_glkhandle);
		if (ACPI_FAILURE(status))
			printf("%s: failed to release global lock\n",
			    sc->sc_dev.dv_xname);
	}
	lockmgr(&sc->sc_lock, LK_RELEASE, NULL);
}

/*
 * acpiec_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpiec_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	return (acpi_match_hid(aa->aa_node->ad_devinfo, ec_hid));
}

void
acpiec_early_attach(struct device *parent)
{
	struct acpi_softc *sc = (struct acpi_softc *)parent;
	EC_BOOT_RESOURCES *ep;
	ACPI_HANDLE handle;
	ACPI_STATUS rv;
	ACPI_INTEGER tmp;

	rv = AcpiGetFirmwareTable("ECDT", 1, ACPI_LOGICAL_ADDRESSING,
	    (void *)&ep);
	if (ACPI_FAILURE(rv))
		return;

	if (ep->EcControl.RegisterBitWidth != 8 ||
	    ep->EcData.RegisterBitWidth != 8) {
		printf("%s: ECDT data is invalid, RegisterBitWidth=%d/%d\n",
		    parent->dv_xname,
		    ep->EcControl.RegisterBitWidth, ep->EcData.RegisterBitWidth);
		return;
	}

	rv = AcpiGetHandle(ACPI_ROOT_OBJECT, ep->EcId, &handle);
	if (ACPI_FAILURE(rv)) {
		printf("%s: failed to look up EC object %s: %s\n",
		    parent->dv_xname,
		    ep->EcId, AcpiFormatException(rv));
		return;
	}

	ecdt_sc = malloc(sizeof(*ecdt_sc), M_ACPI, M_ZERO);

	strcpy(ecdt_sc->sc_dev.dv_xname, "acpiecdt0"); /* XXX */
	ecdt_sc->sc_handle = handle;

	ecdt_sc->sc_gpebit = ep->GpeBit;
	ecdt_sc->sc_uid = ep->Uid;

	ecdt_sc->sc_data_st = sc->sc_iot;
	if (bus_space_map(ecdt_sc->sc_data_st,
		(bus_addr_t)(ep->EcData.Address), 1, 0,
		&ecdt_sc->sc_data_sh) != 0) {
		printf("%s: bus_space_map failed for EC data.\n",
		    parent->dv_xname);
		goto out1;
	}

	ecdt_sc->sc_csr_st = sc->sc_iot;
	if (bus_space_map(ecdt_sc->sc_csr_st,
		(bus_addr_t)(ep->EcControl.Address), 1, 0,
		&ecdt_sc->sc_csr_sh) != 0) {
		printf("%s: bus_space_map failed for EC control.\n",
		    parent->dv_xname);
		goto out2;
	}

	rv = acpi_eval_integer(handle, "_GLK", &tmp);
	if (ACPI_SUCCESS(rv) && tmp == 1)
		ecdt_sc->sc_glk = 1;

	rv = AcpiInstallGpeHandler(NULL, ecdt_sc->sc_gpebit,
	    ACPI_EVENT_EDGE_TRIGGERED, EcGpeHandler, ecdt_sc);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to install GPE handler: %s\n",
		    parent->dv_xname,
		    AcpiFormatException(rv));
		goto out3;
	}

	rv = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
	    ACPI_ADR_SPACE_EC, EcSpaceHandler, EcSpaceSetup, ecdt_sc);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to install address space handler: %s\n",
		    parent->dv_xname,
		    AcpiFormatException(rv));
		goto out4;
	}

	printf("%s: found ECDT, GPE %d\n", parent->dv_xname,
	    ecdt_sc->sc_gpebit);

	lockinit(&ecdt_sc->sc_lock, PWAIT, "eclock", 0, 0);
	simple_lock_init(&ecdt_sc->sc_slock);

	AcpiOsUnmapMemory(ep, ep->Length);
	return;

 out4:
	AcpiRemoveGpeHandler(NULL, ecdt_sc->sc_gpebit, EcGpeHandler);
 out3:
	bus_space_unmap(ecdt_sc->sc_csr_st, ecdt_sc->sc_csr_sh, 1);
 out2:
	bus_space_unmap(ecdt_sc->sc_data_st, ecdt_sc->sc_data_sh, 1);
 out1:
	free(ecdt_sc, M_ACPI);
	ecdt_sc = NULL;

	AcpiOsUnmapMemory(ep, ep->Length);

	return;
}

/*
 * acpiec_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
acpiec_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpi_ec_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	struct acpi_io *io0, *io1;
	ACPI_STATUS rv;
	ACPI_INTEGER v;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	printf(": ACPI Embedded Controller\n");

	lockinit(&sc->sc_lock, PWAIT, "eclock", 0, 0);
	simple_lock_init(&sc->sc_slock);

	sc->sc_handle = aa->aa_node->ad_handle;

	/* Parse our resources. */
	ACPI_DEBUG_PRINT((ACPI_DB_RESOURCES, "parsing EC resources\n"));
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node, &sc->sc_res,
	    &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	rv = acpi_eval_integer(aa->aa_node->ad_handle, "_UID", &v);

	/* check if we already attached EC via ECDT */
	if (ACPI_SUCCESS(rv) && ecdt_sc && ecdt_sc->sc_uid == v) {

		/* detach all ECDT handles */
		rv = AcpiRemoveAddressSpaceHandler(ACPI_ROOT_OBJECT,
		    ACPI_ADR_SPACE_EC, EcSpaceHandler);
		if (ACPI_FAILURE(rv))
			printf("ERROR: RemoveAddressSpaceHandler: %s\n",
			    AcpiFormatException(rv));
		rv = AcpiRemoveGpeHandler(NULL, ecdt_sc->sc_gpebit,
		    EcGpeHandler);
		if (ACPI_FAILURE(rv))
			printf("ERROR: RemoveAddressSpaceHandler: %s\n",
			    AcpiFormatException(rv));

		bus_space_unmap(ecdt_sc->sc_csr_st,
		    ecdt_sc->sc_csr_sh, 1);
		bus_space_unmap(ecdt_sc->sc_data_st,
		    ecdt_sc->sc_data_sh, 1);

		free(ecdt_sc, M_ACPI);
		ecdt_sc = NULL;
	}

	sc->sc_data_st = aa->aa_iot;
	io0 = acpi_res_io(&sc->sc_res, 0);
	if (io0 == NULL) {
		printf("%s: unable to find data register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_data_st, io0->ar_base, io0->ar_length,
	    0, &sc->sc_data_sh) != 0) {
		printf("%s: unable to map data register\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_csr_st = aa->aa_iot;
	io1 = acpi_res_io(&sc->sc_res, 1);
	if (io1 == NULL) {
		printf("%s: unable to find csr register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_csr_st, io1->ar_base, io1->ar_length,
	    0, &sc->sc_csr_sh) != 0) {
		printf("%s: unable to map csr register\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * evaluate _GLK to see if we should acquire global lock
	 * when accessing the EC.
	 */
	rv = acpi_eval_integer(sc->sc_handle, "_GLK", &v);
	if (ACPI_FAILURE(rv)) {
		if (rv != AE_NOT_FOUND)
			printf("%s: unable to evaluate _GLK: %s\n",
			       sc->sc_dev.dv_xname,
			       AcpiFormatException(rv));
		sc->sc_glk = 0;
	} else
		sc->sc_glk = v;

	/*
	 * Install GPE handler.
	 *
	 * We evaluate the _GPE method to find the GPE bit used by the
	 * Embedded Controller to signal status (SCI).
	 */
	rv = acpi_eval_integer(sc->sc_handle, "_GPE", &v);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to evaluate _GPE: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}
	sc->sc_gpebit = v;

	/*
	 * Install a handler for this EC's GPE bit.  Note that EC SCIs are 
	 * treated as both edge- and level-triggered interrupts; in other words
	 * we clear the status bit immediately after getting an EC-SCI, then
	 * again after we're done processing the event.  This guarantees that
	 * events we cause while performing a transaction (e.g. IBE/OBF) get 
	 * cleared before re-enabling the GPE.
	 */
	rv = AcpiInstallGpeHandler(NULL, sc->sc_gpebit,
	     ACPI_EVENT_EDGE_TRIGGERED, EcGpeHandler, sc);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to install GPE handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	/* Install address space handler. */
	rv = AcpiInstallAddressSpaceHandler(sc->sc_handle,
	     ACPI_ADR_SPACE_EC, EcSpaceHandler, EcSpaceSetup, sc);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to install address space handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		(void)AcpiRemoveGpeHandler(NULL, sc->sc_gpebit,
		    EcGpeHandler);
		return;
	}

	return_VOID;
}

static void
EcGpeQueryHandler(void *Context)
{
	struct acpi_ec_softc *sc = Context;
	UINT8 Data;
	ACPI_STATUS Status;
	char qxx[5];

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	for (;;) {
		/*
		 * Check EC_SCI.
		 * 
		 * Bail out if the EC_SCI bit of the status register is not
		 * set. Note that this function should only be called when
		 * this bit is set (polling is used to detect IBE/OBF events).
		 *
		 * It is safe to do this without locking the controller, as
		 * it's OK to call EcQuery when there's no data ready; in the
		 * worst case we should just find nothing waiting for us and
		 * bail.
		 */
		if ((EC_CSR_READ(sc) & EC_EVENT_SCI) == 0)
			break;

		EcLock(sc);

		/*
		 * Find out why the EC is signalling us
		 */
		Status = EcQuery(sc, &Data);

		EcUnlock(sc);
	    
		/*
		 * If we failed to get anything from the EC, give up.
		 */
		if (ACPI_FAILURE(Status)) {
			printf("%s: GPE query failed: %s\n",
			    sc->sc_dev.dv_xname, AcpiFormatException(Status));
			break;
		}

		/*
		 * Evaluate _Qxx to respond to the controller.
		 */
		sprintf(qxx, "_Q%02X", Data);
		Status = AcpiEvaluateObject(sc->sc_handle, qxx,
		    NULL, NULL);

		/*
		 * Ignore spurious query requests.
		 */
		if (ACPI_FAILURE(Status) &&
		    (Data != 0 || Status != AE_NOT_FOUND)) {
			printf("%s: evaluation of GPE query method %s "
			    "failed: %s\n", sc->sc_dev.dv_xname, qxx,
			    AcpiFormatException(Status));
		}
	}

	/* I know I request Level trigger cleanup */
	Status = AcpiClearGpe(NULL, sc->sc_gpebit, ACPI_NOT_ISR);
	if (ACPI_FAILURE(Status))
		printf("%s: AcpiClearGpe failed: %s\n", sc->sc_dev.dv_xname,
		    AcpiFormatException(Status));

	return_VOID;
}

static void
EcGpeHandler(void *Context)
{
	struct acpi_ec_softc *sc = Context;
	uint32_t csrvalue;
	ACPI_STATUS Status;

	/* 
	 * If EC is locked, the intr must process EcRead/Write wait only.
	 * Query request must be pending.
	 */
	simple_lock(&sc->sc_slock);
	if (sc->sc_flags & EC_F_TRANSACTION) {
		csrvalue = EC_CSR_READ(sc);
		if (csrvalue & EC_EVENT_SCI)
			sc->sc_flags |= EC_F_PENDQUERY;

		if ((csrvalue & EC_FLAG_OUTPUT_BUFFER) != 0 ||
		    (csrvalue & EC_FLAG_INPUT_BUFFER) == 0) {
			sc->sc_csrvalue = csrvalue;
			wakeup(&sc->sc_csrvalue);
		}
		simple_unlock(&sc->sc_slock);
	} else {
		simple_unlock(&sc->sc_slock);
		/* Enqueue GpeQuery handler. */
		Status = AcpiOsQueueForExecution(OSD_PRIORITY_HIGH,
		    EcGpeQueryHandler, Context);
		if (ACPI_FAILURE(Status))
			printf("%s: failed to enqueue query handler: %s\n",
			    sc->sc_dev.dv_xname, AcpiFormatException(Status));
	}
}

static ACPI_STATUS
EcSpaceSetup(ACPI_HANDLE Region, UINT32 Function, void *Context,
    void **RegionContext)
{

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	/*
	 * Just pass the context through, there's nothing to do here.
	 */
	if (Function == ACPI_REGION_DEACTIVATE)
		*RegionContext = NULL;
	else
		*RegionContext = Context;

	return_ACPI_STATUS(AE_OK);
}

static ACPI_STATUS
EcSpaceHandler(UINT32 Function, ACPI_PHYSICAL_ADDRESS Address, UINT32 width,
    ACPI_INTEGER *Value, void *Context, void *RegionContext)
{
	struct acpi_ec_softc *sc = Context;
	ACPI_STATUS Status = AE_OK;
	EC_REQUEST EcRequest;
	int i;

	ACPI_FUNCTION_TRACE_U32(__FUNCTION__, (UINT32)Address);

	if ((Address > 0xFF) || (width % 8 != 0) || (Value == NULL) ||
	    (Context == NULL))
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	switch (Function) {
	case ACPI_READ:
		EcRequest.Command = EC_COMMAND_READ;
		EcRequest.Address = Address;
		(*Value) = 0;
		break;

	case ACPI_WRITE:
		EcRequest.Command = EC_COMMAND_WRITE;
		EcRequest.Address = Address;
		break;

	default:
		printf("%s: invalid Address Space function: %d\n",
		    sc->sc_dev.dv_xname, Function);
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Perform the transaction.
	 */
	for (i = 0; i < width; i += 8) {
		if (Function == ACPI_READ)
			EcRequest.Data = 0;
		else
			EcRequest.Data = (UINT8)((*Value) >> i);

		Status = EcTransaction(sc, &EcRequest);
		if (ACPI_FAILURE(Status))
			break;

		(*Value) |= (UINT32)EcRequest.Data << i;
		if (++EcRequest.Address == 0)
			return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	return_ACPI_STATUS(Status);
}

static ACPI_STATUS
EcWaitEventIntr(struct acpi_ec_softc *sc, EC_EVENT Event)
{
	EC_STATUS EcStatus;
	int i;

	ACPI_FUNCTION_TRACE_U32(__FUNCTION__, (UINT32)Event);

	/* XXX Need better test for "yes, you have interrupts". */
	if (cold)
		return_ACPI_STATUS(EcWaitEvent(sc, Event));

	if (EcIsLocked(sc) == 0)
		printf("%s: EcWaitEventIntr called without EC lock!\n",
		    sc->sc_dev.dv_xname);

	EcStatus = EC_CSR_READ(sc);

	/* Too long? */
	for (i = 0; i < 10; i++) {
		/* Check EC status against the desired event. */
		if ((Event == EC_EVENT_OUTPUT_BUFFER_FULL) &&
		    (EcStatus & EC_FLAG_OUTPUT_BUFFER) != 0)
			return_ACPI_STATUS(AE_OK);
      
		if ((Event == EC_EVENT_INPUT_BUFFER_EMPTY) &&
		    (EcStatus & EC_FLAG_INPUT_BUFFER) == 0)
			return_ACPI_STATUS(AE_OK);

		sc->sc_csrvalue = 0;
		/* XXXJRT Sleeping with a lock held? */
		if (tsleep(&sc->sc_csrvalue, 0, "EcWait", 1) != EWOULDBLOCK)
			EcStatus = sc->sc_csrvalue;
		else
			EcStatus = EC_CSR_READ(sc);
	}
	return_ACPI_STATUS(AE_ERROR);
}

static ACPI_STATUS
EcWaitEvent(struct acpi_ec_softc *sc, EC_EVENT Event)
{
	EC_STATUS EcStatus;
	UINT32 i = 0;

	if (EcIsLocked(sc) == 0)
		printf("%s: EcWaitEvent called without EC lock!\n",
		    sc->sc_dev.dv_xname);

	/*
	 * Stall 1us:
	 * ----------
	 * Stall for 1 microsecond before reading the status register
	 * for the first time.  This allows the EC to set the IBF/OBF
	 * bit to its proper state.
	 *
	 * XXX it is not clear why we read the CSR twice.
	 */
	AcpiOsStall(1);
	EcStatus = EC_CSR_READ(sc);

	/*
	 * Wait For Event:
	 * ---------------
	 * Poll the EC status register to detect completion of the last
	 * command.  Wait up to 10ms (in 100us chunks) for this to occur.
	 */
	for (i = 0; i < 100; i++) {
		EcStatus = EC_CSR_READ(sc);

		if ((Event == EC_EVENT_OUTPUT_BUFFER_FULL) &&
		    (EcStatus & EC_FLAG_OUTPUT_BUFFER) != 0)
			return (AE_OK);

		if ((Event == EC_EVENT_INPUT_BUFFER_EMPTY) &&
		    (EcStatus & EC_FLAG_INPUT_BUFFER) == 0)
			return (AE_OK);

		AcpiOsStall(10);
	}

	return (AE_ERROR);
}    

static ACPI_STATUS
EcQuery(struct acpi_ec_softc *sc, UINT8 *Data)
{
	ACPI_STATUS Status;

	if (EcIsLocked(sc) == 0)
		printf("%s: EcQuery called without EC lock!\n",
		    sc->sc_dev.dv_xname);

	EC_CSR_WRITE(sc, EC_COMMAND_QUERY);
	Status = EcWaitEventIntr(sc, EC_EVENT_OUTPUT_BUFFER_FULL);
	if (ACPI_SUCCESS(Status))
		*Data = EC_DATA_READ(sc);

	if (ACPI_FAILURE(Status))
		printf("%s: timed out waiting for EC to respond to "
		    "EC_COMMAND_QUERY\n", sc->sc_dev.dv_xname);

	return (Status);
}    

static ACPI_STATUS
EcTransaction(struct acpi_ec_softc *sc, EC_REQUEST *EcRequest)
{
	ACPI_STATUS Status;

	EcLock(sc);

	/*
	 * Perform the transaction.
	 */
	switch (EcRequest->Command) {
	case EC_COMMAND_READ:
		Status = EcRead(sc, EcRequest->Address, &(EcRequest->Data));
		break;

	case EC_COMMAND_WRITE:
		Status = EcWrite(sc, EcRequest->Address, &(EcRequest->Data));
		break;

	default:
		Status = AE_SUPPORT;
		break;
	}

	EcUnlock(sc);

	return(Status);
}

static ACPI_STATUS
EcRead(struct acpi_ec_softc *sc, UINT8 Address, UINT8 *Data)
{
	ACPI_STATUS Status;

	if (EcIsLocked(sc) == 0)
		printf("%s: EcRead called without EC lock!\n",
		    sc->sc_dev.dv_xname);

	/* EcBurstEnable(EmbeddedController); */

	EC_CSR_WRITE(sc, EC_COMMAND_READ);
	if ((Status = EcWaitEventIntr(sc, EC_EVENT_INPUT_BUFFER_EMPTY)) !=
	    AE_OK) {
		printf("%s: EcRead: timeout waiting for EC to process "
		    "read command\n", sc->sc_dev.dv_xname);
		return (Status);
	}

	EC_DATA_WRITE(sc, Address);
	if ((Status = EcWaitEventIntr(sc, EC_EVENT_OUTPUT_BUFFER_FULL)) !=
	    AE_OK) {
		printf("%s: EcRead: timeout waiting for EC to send data\n",
		    sc->sc_dev.dv_xname);
		return (Status);
	}

	(*Data) = EC_DATA_READ(sc);

	/* EcBurstDisable(EmbeddedController); */

	return (AE_OK);
}    

static ACPI_STATUS
EcWrite(struct acpi_ec_softc *sc, UINT8 Address, UINT8 *Data)
{
	ACPI_STATUS Status;

	if (EcIsLocked(sc) == 0)
		printf("%s: EcWrite called without EC lock!\n",
		    sc->sc_dev.dv_xname);

	/* EcBurstEnable(EmbeddedController); */

	EC_CSR_WRITE(sc, EC_COMMAND_WRITE);
	if ((Status = EcWaitEventIntr(sc, EC_EVENT_INPUT_BUFFER_EMPTY)) !=
	    AE_OK) {
		printf("%s: EcWrite: timeout waiting for EC to process "
		    "write command\n", sc->sc_dev.dv_xname);
		return (Status);
	}

	EC_DATA_WRITE(sc, Address);
	if ((Status = EcWaitEventIntr(sc, EC_EVENT_INPUT_BUFFER_EMPTY)) !=
	    AE_OK) {
		printf("%s: EcWrite: timeout waiting for EC to process "
		    "address\n", sc->sc_dev.dv_xname);
		return (Status);
	}

	EC_DATA_WRITE(sc, *Data);
	if ((Status = EcWaitEventIntr(sc, EC_EVENT_INPUT_BUFFER_EMPTY)) !=
	    AE_OK) {
		printf("%s: EcWrite: timeout waiting for EC to process "
		    "data\n", sc->sc_dev.dv_xname);
		return (Status);
	}

	/* EcBurstDisable(EmbeddedController); */

	return (AE_OK);
}
