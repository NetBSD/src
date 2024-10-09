/*	$NetBSD: apei_einj.c,v 1.7.4.2 2024/10/09 13:00:11 martin Exp $	*/

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
 * APEI EINJ -- Error Injection Table
 *
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#error-injection
 *
 * XXX Consider a /dev node with ioctls for error injection rather than
 * the somewhat kooky sysctl interface.  By representing an error
 * injection request in a structure, we can serialize access to the
 * platform's EINJ operational context.  However, this also requires
 * some nontrivial userland support; maybe relying on the user to tread
 * carefully with error injection is fine -- after all, many types of
 * error injection will cause a system halt/panic.
 *
 * XXX Properly expose SET_ERROR_TYPE_WITH_ADDRESS, which has a more
 * complicated relationship with its RegisterRegion field.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_einj.c,v 1.7.4.2 2024/10/09 13:00:11 martin Exp $");

#include <sys/types.h>

#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_einjvar.h>
#include <dev/acpi/apei_interp.h>
#include <dev/acpi/apei_mapreg.h>
#include <dev/acpi/apei_reg.h>
#include <dev/acpi/apeivar.h>

#include "ioconf.h"

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("apei")

static void apei_einj_instfunc(ACPI_WHEA_HEADER *, struct apei_mapreg *,
    void *, uint32_t *, uint32_t);
static uint64_t apei_einj_act(struct apei_softc *, enum AcpiEinjActions,
    uint64_t);
static uint64_t apei_einj_trigger(struct apei_softc *, uint64_t);
static int apei_einj_action_sysctl(SYSCTLFN_ARGS);
static int apei_einj_trigger_sysctl(SYSCTLFN_ARGS);
static int apei_einj_types_sysctl(SYSCTLFN_ARGS);

/*
 * apei_einj_action
 *
 *	Symbolic names of the APEI EINJ (Error Injection) logical actions
 *	are taken (and downcased) from:
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#error-injection-actions
 */
static const char *const apei_einj_action[] = {
	[ACPI_EINJ_BEGIN_OPERATION] = "begin_injection_operation",
	[ACPI_EINJ_GET_TRIGGER_TABLE] = "get_trigger_error_action_table",
	[ACPI_EINJ_SET_ERROR_TYPE] = "set_error_type",
	[ACPI_EINJ_GET_ERROR_TYPE] = "get_error_type",
	[ACPI_EINJ_END_OPERATION] = "end_operation",
	[ACPI_EINJ_EXECUTE_OPERATION] = "execute_operation",
	[ACPI_EINJ_CHECK_BUSY_STATUS] = "check_busy_status",
	[ACPI_EINJ_GET_COMMAND_STATUS] = "get_command_status",
	[ACPI_EINJ_SET_ERROR_TYPE_WITH_ADDRESS] = "set_error_type_with_address",
	[ACPI_EINJ_GET_EXECUTE_TIMINGS] = "get_execute_operation_timings",
};

/*
 * apei_einj_instruction
 *
 *	Symbolic names of the APEI EINJ (Error Injection) instructions to
 *	implement logical actions are taken (and downcased) from:
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#injection-instructions-table
 */

static const char *const apei_einj_instruction[] = {
	[ACPI_EINJ_READ_REGISTER] = "read_register",
	[ACPI_EINJ_READ_REGISTER_VALUE] = "read_register",
	[ACPI_EINJ_WRITE_REGISTER] = "write_register",
	[ACPI_EINJ_WRITE_REGISTER_VALUE] = "write_register_value",
	[ACPI_EINJ_NOOP] = "noop",
};

/*
 * apei_einj_instreg
 *
 *	Table of which instructions use a register operand.
 *
 *	Must match apei_einj_instfunc.
 */
static const bool apei_einj_instreg[] = {
	[ACPI_EINJ_READ_REGISTER] = true,
	[ACPI_EINJ_READ_REGISTER_VALUE] = true,
	[ACPI_EINJ_WRITE_REGISTER] = true,
	[ACPI_EINJ_WRITE_REGISTER_VALUE] = true,
	[ACPI_EINJ_NOOP] = false,
};

/*
 * apei_einj_attach(sc)
 *
 *	Scan the Error Injection table to ascertain what error
 *	injection actions the firmware supports and how to perform
 *	them.  Create sysctl nodes for triggering error injection.
 */
void
apei_einj_attach(struct apei_softc *sc)
{
	ACPI_TABLE_EINJ *einj = sc->sc_tab.einj;
	struct apei_einj_softc *jsc = &sc->sc_einj;
	ACPI_EINJ_ENTRY *entry;
	const struct sysctlnode *sysctl_einj;
	const struct sysctlnode *sysctl_einj_action;
	uint32_t i, nentries, maxnentries;
	unsigned action;
	int error;

	/*
	 * Verify the table length, table header length, and
	 * instruction entry count are all sensible.  If the header is
	 * truncated, stop here; if the entries are truncated, stop at
	 * the largest integral number of full entries that fits.
	 */
	if (einj->Header.Length < sizeof(*einj)) {
		aprint_error_dev(sc->sc_dev, "EINJ: truncated table:"
		    " %"PRIu32" < %zu minimum bytes\n",
		    einj->Header.Length, sizeof(*einj));
		return;
	}
	if (einj->HeaderLength <
	    sizeof(*einj) - offsetof(ACPI_TABLE_EINJ, HeaderLength)) {
		aprint_error_dev(sc->sc_dev, "EINJ: truncated header:"
		    " %"PRIu32" < %zu bytes\n",
		    einj->HeaderLength,
		    sizeof(*einj) - offsetof(ACPI_TABLE_EINJ, HeaderLength));
		return;
	}
	nentries = einj->Entries;
	maxnentries = (einj->Header.Length - sizeof(*einj))/sizeof(*entry);
	if (nentries > maxnentries) {
		aprint_error_dev(sc->sc_dev, "EINJ: excessive entries:"
		    " %"PRIu32", truncating to %"PRIu32"\n",
		    nentries, maxnentries);
		nentries = maxnentries;
	}
	if (nentries*sizeof(*entry) < einj->Header.Length - sizeof(*einj)) {
		aprint_error_dev(sc->sc_dev, "EINJ:"
		    " %zu bytes of trailing garbage after last entry\n",
		    einj->Header.Length - nentries*sizeof(*entry));
	}

	/*
	 * Create sysctl hw.acpi.apei.einj for all EINJ-related knobs.
	 */
	error = sysctl_createv(&sc->sc_sysctllog, 0,
	    &sc->sc_sysctlroot, &sysctl_einj, 0,
	    CTLTYPE_NODE, "einj",
	    SYSCTL_DESCR("Error injection"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to create"
		    " hw.acpi.apei.einj: %d\n", error);
		sysctl_einj = NULL;
	}

	/*
	 * Create an interpreter for EINJ actions.
	 */
	jsc->jsc_interp = apei_interp_create("EINJ",
	    apei_einj_action, __arraycount(apei_einj_action),
	    apei_einj_instruction, __arraycount(apei_einj_instruction),
	    apei_einj_instreg, /*instvalid*/NULL, apei_einj_instfunc);

	/*
	 * Compile the interpreter from the EINJ action instruction
	 * table.
	 */
	entry = (ACPI_EINJ_ENTRY *)(einj + 1);
	for (i = 0; i < nentries; i++, entry++)
		apei_interp_pass1_load(jsc->jsc_interp, i, &entry->WheaHeader);
	entry = (ACPI_EINJ_ENTRY *)(einj + 1);
	for (i = 0; i < nentries; i++, entry++) {
		apei_interp_pass2_verify(jsc->jsc_interp, i,
		    &entry->WheaHeader);
	}
	apei_interp_pass3_alloc(jsc->jsc_interp);
	entry = (ACPI_EINJ_ENTRY *)(einj + 1);
	for (i = 0; i < nentries; i++, entry++) {
		apei_interp_pass4_assemble(jsc->jsc_interp, i,
		    &entry->WheaHeader);
	}
	apei_interp_pass5_verify(jsc->jsc_interp);

	/*
	 * Create sysctl hw.acpi.apei.einj.action for individual actions.
	 */
	error = sysctl_einj == NULL ? ENOENT :
	    sysctl_createv(&sc->sc_sysctllog, 0,
		&sysctl_einj, &sysctl_einj_action, 0,
		CTLTYPE_NODE, "action",
		SYSCTL_DESCR("EINJ actions"),
		NULL, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to create"
		    " hw.acpi.apei.einj.action: %d\n", error);
		sysctl_einj_action = NULL;
	}

	/*
	 * Create sysctl nodes for each action we know about.
	 */
	for (action = 0; action < __arraycount(apei_einj_action); action++) {
		if (apei_einj_action[action] == NULL)
			continue;

		/*
		 * Check to see if there are any instructions for this
		 * action.
		 *
		 * XXX Maybe add this to the apei_interp.h abstraction.
		 */
		entry = (ACPI_EINJ_ENTRY *)(einj + 1);
		for (i = 0; i < nentries; i++, entry++) {
			ACPI_WHEA_HEADER *const header = &entry->WheaHeader;

			if (action == header->Action)
				break;
		}
		if (i == nentries) {
			/*
			 * No instructions for this action, so assume
			 * it's not supported.
			 */
			continue;
		}

		/*
		 * Create a sysctl knob to perform the action.
		 */
		error = sysctl_einj_action == NULL ? ENOENT :
		    sysctl_createv(&sc->sc_sysctllog, 0,
			&sysctl_einj_action, NULL, CTLFLAG_READWRITE,
			CTLTYPE_QUAD, apei_einj_action[action],
			NULL,	/* description */
			&apei_einj_action_sysctl, 0, NULL, 0,
			action, CTL_EOL);
		if (error) {
			aprint_error_dev(sc->sc_dev, "failed to create"
			    " sysctl hw.acpi.apei.einj.action.%s: %d\n",
			    apei_einj_action[action], error);
			continue;
		}
	}

	/*
	 * Create a sysctl knob to trigger error.
	 */
	error = sysctl_einj == NULL ? ENOENT :
	    sysctl_createv(&sc->sc_sysctllog, 0,
		&sysctl_einj, NULL, CTLFLAG_READWRITE,
		CTLTYPE_QUAD, "trigger",
		NULL,	/* description */
		&apei_einj_trigger_sysctl, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to create"
		    " sysctl hw.acpi.apei.einj.trigger: %d\n",
		    error);
	}

	/*
	 * Query the available types of error to inject and print it to
	 * dmesg.
	 *
	 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#error-types
	 */
	uint64_t types = apei_einj_act(sc, ACPI_EINJ_GET_ERROR_TYPE, 0);
	char typesbuf[1024], *typesp;
	/* XXX define this format somewhere */
	snprintb_m(typesbuf, sizeof(typesbuf), "\177\020"
	    "b\000"	"PROC_CORRECTABLE\0"
	    "b\001"	"PROC_UNCORRECTABLE\0"
	    "b\002"	"PROC_FATAL\0"
	    "b\003"	"MEM_CORRECTABLE\0"
	    "b\004"	"MEM_UNCORRECTABLE\0"
	    "b\005"	"MEM_FATAL\0"
	    "b\006"	"PCIE_CORRECTABLE\0"
	    "b\007"	"PCIE_UNCORRECTABLE\0"
	    "b\010"	"PCIE_FATAL\0"
	    "b\011"	"PLAT_CORRECTABLE\0"
	    "b\012"	"PLAT_UNCORRECTABLE\0"
	    "b\013"	"PLAT_FATAL\0"
	    "b\014"	"CXLCACHE_CORRECTABLE\0"
	    "b\015"	"CXLCACHE_UNCORRECTABLE\0"
	    "b\016"	"CXLCACHE_FATAL\0"
	    "b\017"	"CXLMEM_CORRECTABLE\0"
	    "b\020"	"CXLMEM_UNCORRECTABLE\0"
	    "b\021"	"CXLMEM_FATAL\0"
//	    "f\022\014"	"reserved\0"
	    "b\036"	"EINJv2\0"
	    "b\037"	"VENDOR\0"
	    "\0", types, 36);
	for (typesp = typesbuf; strlen(typesp); typesp += strlen(typesp) + 1) {
		aprint_normal_dev(sc->sc_dev, "EINJ: can inject:"
		    " %s\n", typesp);
	}

	/*
	 * Create a sysctl knob to query the available types of error
	 * to inject.  In principle this could change dynamically, so
	 * we'll make it dynamic.
	 */
	error = sysctl_einj == NULL ? ENOENT :
	    sysctl_createv(&sc->sc_sysctllog, 0,
		&sysctl_einj, NULL, 0,
		CTLTYPE_QUAD, "types",
		SYSCTL_DESCR("Types of errors that can be injected"),
		&apei_einj_types_sysctl, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to create"
		    " sysctl hw.acpi.apei.einj.types: %d\n",
		    error);
	}
}

/*
 * apei_einj_detach(sc)
 *
 *	Free any software resources associated with the Error Injection
 *	table.
 */
void
apei_einj_detach(struct apei_softc *sc)
{
	struct apei_einj_softc *jsc = &sc->sc_einj;

	if (jsc->jsc_interp) {
		apei_interp_destroy(jsc->jsc_interp);
		jsc->jsc_interp = NULL;
	}
}

/*
 * struct apei_einj_machine
 *
 *	Machine state for executing EINJ instructions.
 */
struct apei_einj_machine {
	struct apei_softc	*sc;
	uint64_t		x;	/* in */
	uint64_t		y;	/* out */
};

/*
 * apei_einj_instfunc(header, cookie, &ip, maxip)
 *
 *	Run a single instruction in the service of performing an EINJ
 *	action.  Updates the EINJ machine at cookie in place.
 *
 *	This doesn't read or write ip.  The TRIGGER_ERROR logic relies
 *	on this; if you change the fact, you must update that logic
 *	too.
 */
static void
apei_einj_instfunc(ACPI_WHEA_HEADER *header, struct apei_mapreg *map,
    void *cookie, uint32_t *ipp, uint32_t maxip)
{
	struct apei_einj_machine *M = cookie;

	/*
	 * Abbreviate some of the intermediate quantities to make the
	 * instruction logic conciser and more legible.
	 */
	const uint8_t BitOffset = header->RegisterRegion.BitOffset;
	const uint64_t Mask = header->Mask;
	const uint64_t Value = header->Value;
	ACPI_GENERIC_ADDRESS *const reg = &header->RegisterRegion;
	const bool preserve_register = header->Flags & ACPI_EINJ_PRESERVE;

	aprint_debug_dev(M->sc->sc_dev, "%s: instr=0x%02"PRIx8
	    " (%s)"
	    " Address=0x%"PRIx64
	    " BitOffset=%"PRIu8" Mask=0x%"PRIx64" Value=0x%"PRIx64
	    " Flags=0x%"PRIx8"\n",
	    __func__, header->Instruction,
	    (header->Instruction < __arraycount(apei_einj_instruction)
		? apei_einj_instruction[header->Instruction]
		: "unknown"),
	    reg->Address,
	    BitOffset, Mask, Value,
	    header->Flags);

	/*
	 * Zero-initialize the output by default.
	 */
	M->y = 0;

	/*
	 * Dispatch the instruction.
	 */
	switch (header->Instruction) {
	case ACPI_EINJ_READ_REGISTER:
		M->y = apei_read_register(reg, map, Mask);
		break;
	case ACPI_EINJ_READ_REGISTER_VALUE: {
		uint64_t v;

		v = apei_read_register(reg, map, Mask);
		M->y = (v == Value ? 1 : 0);
		break;
	}
	case ACPI_EINJ_WRITE_REGISTER:
		apei_write_register(reg, map, Mask, preserve_register, M->x);
		break;
	case ACPI_EINJ_WRITE_REGISTER_VALUE:
		apei_write_register(reg, map, Mask, preserve_register, Value);
		break;
	case ACPI_EINJ_NOOP:
		break;
	default:		/* XXX unreachable */
		break;
	}
}

/*
 * apei_einj_act(sc, action, x)
 *
 *	Perform the named EINJ action with input x, by executing the
 *	instruction defined for the action by the EINJ, and return the
 *	output.
 */
static uint64_t
apei_einj_act(struct apei_softc *sc, enum AcpiEinjActions action,
    uint64_t x)
{
	struct apei_einj_softc *const jsc = &sc->sc_einj;
	struct apei_einj_machine einj_machine, *const M = &einj_machine;

	aprint_debug_dev(sc->sc_dev, "%s: action=%d (%s) input=0x%"PRIx64"\n",
	    __func__,
	    action,
	    (action < __arraycount(apei_einj_action)
		? apei_einj_action[action]
		: "unknown"),
	    x);

	/*
	 * Initialize the machine to execute the action's instructions.
	 */
	memset(M, 0, sizeof(*M));
	M->sc = sc;
	M->x = x;		/* input */
	M->y = 0;		/* output */

	/*
	 * Run the interpreter.
	 */
	apei_interpret(jsc->jsc_interp, action, M);

	/*
	 * Return the result.
	 */
	aprint_debug_dev(sc->sc_dev, "%s: output=0x%"PRIx64"\n", __func__,
	    M->y);
	return M->y;
}

/*
 * apei_einj_trigger(sc, x)
 *
 *	Obtain the TRIGGER_ERROR action table and, if there is anything
 *	to be done with it, execute it with input x and return the
 *	output.  If nothing is to be done, return 0.
 */
static uint64_t
apei_einj_trigger(struct apei_softc *sc, uint64_t x)
{
	uint64_t teatab_pa;
	ACPI_EINJ_TRIGGER *teatab = NULL;
	size_t mapsize = 0, tabsize, bodysize;
	ACPI_EINJ_ENTRY *entry;
	struct apei_einj_machine einj_machine, *const M = &einj_machine;
	uint32_t i, nentries;

	/*
	 * Initialize the machine to execute the TRIGGER_ERROR action's
	 * instructions.  Do this early to keep the error branches
	 * simpler.
	 */
	memset(M, 0, sizeof(*M));
	M->sc = sc;
	M->x = x;		/* input */
	M->y = 0;		/* output */

	/*
	 * Get the TRIGGER_ERROR action table's physical address.
	 */
	teatab_pa = apei_einj_act(sc, ACPI_EINJ_GET_TRIGGER_TABLE, 0);

	/*
	 * Map just the header.  We don't know how large the table is
	 * because we get that from the header.
	 */
	mapsize = sizeof(*teatab);
	teatab = AcpiOsMapMemory(teatab_pa, mapsize);

	/*
	 * If there's no entries, stop here -- nothing to do separately
	 * to trigger an error report.
	 */
	nentries = teatab->EntryCount;
	if (nentries == 0)
		goto out;

	/*
	 * If the header size or the table size is nonsense, bail.
	 */
	if (teatab->HeaderSize < sizeof(*teatab) ||
	    teatab->TableSize < teatab->HeaderSize) {
		device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
		    " invalid sizes:"
		    " HeaderSize=%"PRIu32" TableSize=%"PRIu32"\n",
		    teatab->HeaderSize, teatab->TableSize);
	}

	/*
	 * If the revision is nonzero, we don't know what to do.  I've
	 * only seen revision zero so far, and the spec doesn't say
	 * anything about revisions that I've found.
	 */
	if (teatab->Revision != 0) {
		device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
		    " unknown revision: %"PRIx32"\n", teatab->Revision);
		goto out;
	}

	/*
	 * Truncate the table to the number of entries requested and
	 * ignore trailing garbage if the table is long, or round the
	 * number of entries down to what fits in the table if the
	 * table is short.
	 */
	tabsize = teatab->TableSize;
	bodysize = tabsize - teatab->HeaderSize;
	if (nentries < howmany(bodysize, sizeof(ACPI_EINJ_ENTRY))) {
		device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
		    " %zu bytes of trailing garbage\n",
		    tabsize - nentries*sizeof(ACPI_EINJ_ENTRY));
		bodysize = nentries*sizeof(ACPI_EINJ_ENTRY);
		tabsize = teatab->HeaderSize + bodysize;
	} else if (nentries > howmany(bodysize, sizeof(ACPI_EINJ_ENTRY))) {
		device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
		    " truncated to %zu entries\n",
		    nentries*sizeof(ACPI_EINJ_ENTRY));
		nentries = howmany(bodysize, sizeof(ACPI_EINJ_ENTRY));
		bodysize = nentries*sizeof(ACPI_EINJ_ENTRY);
		tabsize = teatab->HeaderSize + bodysize;
	}

	/*
	 * Unmap the header and map the whole table instead.
	 */
	AcpiOsUnmapMemory(teatab, mapsize);
	mapsize = tabsize;
	teatab = AcpiOsMapMemory(teatab_pa, mapsize);

	/*
	 * Now iterate over the EINJ-type entries and execute the
	 * trigger error action instructions -- but skip if they're not
	 * for the TRIGGER_ERROR action, and stop if they're truncated.
	 *
	 * Entries are fixed-size, so we can just index them.
	 */
	entry = (ACPI_EINJ_ENTRY *)((char *)teatab + teatab->HeaderSize);
	for (i = 0; i < nentries; i++) {
		ACPI_WHEA_HEADER *const header = &entry[i].WheaHeader;

		/*
		 * Verify the action is TRIGGER_ERROR.  If not, skip.
		 */
		if (header->Action != ACPI_EINJ_TRIGGER_ERROR) {
			device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
			    " other action: %"PRIu32" (%s)\n",
			    header->Action,
			    (header->Action < __arraycount(apei_einj_action)
				? apei_einj_action[header->Action]
				: "unknown"));
			continue;
		}

		/*
		 * Verify the instruction.
		 */
		if (header->Instruction >= __arraycount(apei_einj_instreg)) {
			device_printf(sc->sc_dev, "TRIGGER_ERROR action table:"
			    " unknown instruction: %"PRIu32"\n",
			    header->Instruction);
			continue;
		}

		/*
		 * Map the register if needed.
		 */
		struct apei_mapreg *map = NULL;
		if (apei_einj_instreg[header->Instruction]) {
			map = apei_mapreg_map(&header->RegisterRegion);
			if (map == NULL) {
				device_printf(sc->sc_dev, "TRIGGER_ERROR"
				    " action table: failed to map register\n");
				continue;
			}
		}

		/*
		 * Execute the instruction.  Since there's only one
		 * action, we don't bother with the apei_interp
		 * machinery to collate instruction tables for each
		 * action.  EINJ instructions don't change ip.
		 */
		uint32_t ip = i + 1;
		apei_einj_instfunc(header, map, M, &ip, nentries);
		KASSERT(ip == i + 1);

		/*
		 * Unmap the register if mapped.
		 */
		if (map != NULL)
			apei_mapreg_unmap(&header->RegisterRegion, map);
	}

out:	if (teatab) {
		AcpiOsUnmapMemory(teatab, mapsize);
		teatab = NULL;
		mapsize = 0;
	}
	return M->y;
}

/*
 * apei_einj_action_sysctl:
 *
 *	Handle sysctl queries under hw.acpi.apei.einj.action.*.
 */
static int
apei_einj_action_sysctl(SYSCTLFN_ARGS)
{
	device_t apei0 = NULL;
	struct apei_softc *sc;
	enum AcpiEinjActions action;
	struct sysctlnode node = *rnode;
	uint64_t v;
	int error;

	/*
	 * As a defence against mistakes, require the user to specify a
	 * write.
	 */
	if (newp == NULL) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Take a reference to the apei0 device so it doesn't go away
	 * while we're working, and get the softc.
	 */
	if ((apei0 = device_lookup_acquire(&apei_cd, 0)) == NULL) {
		error = ENXIO;
		goto out;
	}
	sc = device_private(apei0);

	/*
	 * Fail if there's no EINJ.
	 */
	if (sc->sc_tab.einj == NULL) {
		error = ENODEV;
		goto out;
	}

	/*
	 * Identify the requested action.  If we don't recognize it,
	 * fail with EINVAL.
	 */
	switch (node.sysctl_num) {
	case ACPI_EINJ_BEGIN_OPERATION:
	case ACPI_EINJ_GET_TRIGGER_TABLE:
	case ACPI_EINJ_SET_ERROR_TYPE:
	case ACPI_EINJ_GET_ERROR_TYPE:
	case ACPI_EINJ_END_OPERATION:
	case ACPI_EINJ_EXECUTE_OPERATION:
	case ACPI_EINJ_CHECK_BUSY_STATUS:
	case ACPI_EINJ_GET_COMMAND_STATUS:
	case ACPI_EINJ_SET_ERROR_TYPE_WITH_ADDRESS:
	case ACPI_EINJ_GET_EXECUTE_TIMINGS:
		action = node.sysctl_num;
		break;
	default:
		error = ENOENT;
		goto out;
	}

	/*
	 * Kludge: Copy the `new value' for the sysctl in as an input
	 * to the injection action.
	 */
	error = sysctl_copyin(curlwp, newp, &v, sizeof(v));
	if (error)
		goto out;

	/*
	 * Perform the EINJ action by following the table's
	 * instructions.
	 */
	v = apei_einj_act(sc, action, v);

	/*
	 * Return the output of the operation as the `old value' of the
	 * sysctl.  This also updates v with what was written to the
	 * sysctl was written, but we don't care because we already
	 * read that in and acted on it.
	 */
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

out:	if (apei0) {
		device_release(apei0);
		apei0 = NULL;
	}
	return error;
}

/*
 * apei_einj_trigger_sysctl
 *
 *	Handle sysctl hw.acpi.apei.einj.trigger.
 */
static int
apei_einj_trigger_sysctl(SYSCTLFN_ARGS)
{
	device_t apei0 = NULL;
	struct apei_softc *sc;
	struct sysctlnode node = *rnode;
	uint64_t v;
	int error;

	/*
	 * As a defence against mistakes, require the user to specify a
	 * write.
	 */
	if (newp == NULL) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Take a reference to the apei0 device so it doesn't go away
	 * while we're working, and get the softc.
	 */
	if ((apei0 = device_lookup_acquire(&apei_cd, 0)) == NULL) {
		error = ENXIO;
		goto out;
	}
	sc = device_private(apei0);

	/*
	 * Fail if there's no EINJ.
	 */
	if (sc->sc_tab.einj == NULL) {
		error = ENODEV;
		goto out;
	}

	/*
	 * Kludge: Copy the `new value' for the sysctl in as an input
	 * to the trigger action.
	 */
	error = sysctl_copyin(curlwp, newp, &v, sizeof(v));
	if (error)
		goto out;

	/*
	 * Perform the TRIGGER_ERROR action.
	 */
	v = apei_einj_trigger(sc, v);

	/*
	 * Return the output of the operation as the `old value' of the
	 * sysctl.  This also updates v with what was written to the
	 * sysctl was written, but we don't care because we already
	 * read that in and acted on it.
	 */
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

out:	if (apei0) {
		device_release(apei0);
		apei0 = NULL;
	}
	return error;
}

/*
 * apei_einj_types_sysctl
 *
 *	Handle sysctl hw.acpi.apei.einj.types.
 */
static int
apei_einj_types_sysctl(SYSCTLFN_ARGS)
{
	device_t apei0 = NULL;
	struct apei_softc *sc;
	struct sysctlnode node = *rnode;
	uint64_t types;
	int error;

	/*
	 * Take a reference to the apei0 device so it doesn't go away
	 * while we're working, and get the softc.
	 *
	 * XXX Is this necessary?  Shouldn't sysctl_teardown take care
	 * of preventing new sysctl calls and waiting until all pending
	 * sysctl calls are done?
	 */
	if ((apei0 = device_lookup_acquire(&apei_cd, 0)) == NULL) {
		error = ENXIO;
		goto out;
	}
	sc = device_private(apei0);

	/*
	 * Fail if there's no EINJ.
	 */
	if (sc->sc_tab.einj == NULL) {
		error = ENODEV;
		goto out;
	}

	/*
	 * Perform the GET_ERROR_TYPE action and return the value to
	 * sysctl.
	 *
	 * XXX Should this do it between BEGIN_INJECTION_OPERATION and
	 * END_OPERATION?
	 */
	types = apei_einj_act(sc, ACPI_EINJ_GET_ERROR_TYPE, 0);
	node.sysctl_data = &types;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));

out:	if (apei0) {
		device_release(apei0);
		apei0 = NULL;
	}
	return error;
}
