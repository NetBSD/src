/*	$NetBSD: apei_erst.c,v 1.3.4.2 2024/10/09 13:00:12 martin Exp $	*/

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
 * APEI ERST -- Error Record Serialization Table
 *
 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#error-serialization
 *
 * XXX Expose this through a /dev node with ioctls and/or through a
 * file system.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_erst.c,v 1.3.4.2 2024/10/09 13:00:12 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/systm.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_erstvar.h>
#include <dev/acpi/apei_interp.h>
#include <dev/acpi/apei_reg.h>
#include <dev/acpi/apeivar.h>

#define	_COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("apei")

static bool apei_erst_instvalid(ACPI_WHEA_HEADER *, uint32_t, uint32_t);
static void apei_erst_instfunc(ACPI_WHEA_HEADER *, struct apei_mapreg *,
    void *, uint32_t *, uint32_t);
static uint64_t apei_erst_act(struct apei_softc *, enum AcpiErstActions,
    uint64_t);

/*
 * apei_erst_action
 *
 *	Symbolic names of the APEI ERST (Error Record Serialization
 *	Table) logical actions are taken (and downcased) from:
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#error-record-serialization-actions-table
 */
static const char *const apei_erst_action[] = {
	[ACPI_ERST_BEGIN_WRITE] = "begin_write_operation",
	[ACPI_ERST_BEGIN_READ] = "begin_read_operation",
	[ACPI_ERST_BEGIN_CLEAR] = "begin_clear_operation",
	[ACPI_ERST_END] = "end_operation",
	[ACPI_ERST_SET_RECORD_OFFSET] = "set_record_offset",
	[ACPI_ERST_EXECUTE_OPERATION] = "execute_operation",
	[ACPI_ERST_CHECK_BUSY_STATUS] = "check_busy_status",
	[ACPI_ERST_GET_COMMAND_STATUS] = "get_command_status",
	[ACPI_ERST_GET_RECORD_ID] = "get_record_identifier",
	[ACPI_ERST_SET_RECORD_ID] = "set_record_identifier",
	[ACPI_ERST_GET_RECORD_COUNT] = "get_record_count",
	[ACPI_ERST_BEGIN_DUMMY_WRIITE] = "begin_dummy_write_operation",
	[ACPI_ERST_NOT_USED] = "reserved",
	[ACPI_ERST_GET_ERROR_RANGE] = "get_error_log_address_range",
	[ACPI_ERST_GET_ERROR_LENGTH] = "get_error_log_address_range_length",
	[ACPI_ERST_GET_ERROR_ATTRIBUTES] =
	    "get_error_log_address_range_attributes",
	[ACPI_ERST_EXECUTE_TIMINGS] = "get_execute_operations_timings",
};

/*
 * apei_erst_instruction
 *
 *	Symbolic names of the APEI ERST (Error Record Serialization
 *	Table) instructions to implement logical actions are taken (and
 *	downcased) from:
 *
 *	https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#serialization-instructions
 */
static const char *apei_erst_instruction[] = {
	[ACPI_ERST_READ_REGISTER] = "read_register",
	[ACPI_ERST_READ_REGISTER_VALUE] = "read_register_value",
	[ACPI_ERST_WRITE_REGISTER] = "write_register",
	[ACPI_ERST_WRITE_REGISTER_VALUE] = "write_register_value",
	[ACPI_ERST_NOOP] = "noop",
	[ACPI_ERST_LOAD_VAR1] = "load_var1",
	[ACPI_ERST_LOAD_VAR2] = "load_var2",
	[ACPI_ERST_STORE_VAR1] = "store_var1",
	[ACPI_ERST_ADD] = "add",
	[ACPI_ERST_SUBTRACT] = "subtract",
	[ACPI_ERST_ADD_VALUE] = "add_value",
	[ACPI_ERST_SUBTRACT_VALUE] = "subtract_value",
	[ACPI_ERST_STALL] = "stall",
	[ACPI_ERST_STALL_WHILE_TRUE] = "stall_while_true",
	[ACPI_ERST_SKIP_NEXT_IF_TRUE] = "skip_next_instruction_if_true",
	[ACPI_ERST_GOTO] = "goto",
	[ACPI_ERST_SET_SRC_ADDRESS_BASE] = "set_src_address_base",
	[ACPI_ERST_SET_DST_ADDRESS_BASE] = "set_dst_address_base",
	[ACPI_ERST_MOVE_DATA] = "move_data",
};

/*
 * apei_erst_instreg
 *
 *	Table of which isntructions use a register operand.
 *
 *	Must match apei_erst_instfunc.
 */
static const bool apei_erst_instreg[] = {
	[ACPI_ERST_READ_REGISTER] = true,
	[ACPI_ERST_READ_REGISTER_VALUE] = true,
	[ACPI_ERST_WRITE_REGISTER] = true,
	[ACPI_ERST_WRITE_REGISTER_VALUE] = true,
	[ACPI_ERST_NOOP] = false,
	[ACPI_ERST_LOAD_VAR1] = true,
	[ACPI_ERST_LOAD_VAR2] = true,
	[ACPI_ERST_STORE_VAR1] = true,
	[ACPI_ERST_ADD] = false,
	[ACPI_ERST_SUBTRACT] = false,
	[ACPI_ERST_ADD_VALUE] = true,
	[ACPI_ERST_SUBTRACT_VALUE] = true,
	[ACPI_ERST_STALL] = false,
	[ACPI_ERST_STALL_WHILE_TRUE] = true,
	[ACPI_ERST_SKIP_NEXT_IF_TRUE] = true,
	[ACPI_ERST_GOTO] = false,
	[ACPI_ERST_SET_SRC_ADDRESS_BASE] = true,
	[ACPI_ERST_SET_DST_ADDRESS_BASE] = true,
	[ACPI_ERST_MOVE_DATA] = true,
};

/*
 * XXX dtrace and kernhist
 */
static void
apei_pmemmove(uint64_t pdst, uint64_t psrc, uint64_t nbytes)
{
	char *vdst, *vsrc;

	aprint_debug("ERST: move"
	    " %"PRIu64" bytes from 0x%"PRIx64" to 0x%"PRIx64"\n",
	    nbytes, psrc, pdst);

	/*
	 * Carefully check for overlap.
	 */
	if (pdst == psrc) {
		/*
		 * Technically this could happen, I guess!
		 */
		return;
	} else if (pdst < psrc && psrc < pdst + nbytes) {
		/*
		 *       psrc ______ psrc + nbytes
		 *           /      \
		 * <--------------------->
		 *      \______/
		 *  pdst        pdst + nbytes
		 */
		vdst = AcpiOsMapMemory(pdst, nbytes + (psrc - pdst));
		vsrc = vdst + (psrc - pdst);
		memmove(vdst, vsrc, nbytes);
		AcpiOsUnmapMemory(vdst, nbytes + (psrc - pdst));
	} else if (psrc < pdst && pdst < psrc + nbytes) {
		/*
		 *  psrc ______ psrc + nbytes
		 *      /      \
		 * <--------------------->
		 *           \______/
		 *       pdst        pdst + nbytes
		 */
		vsrc = AcpiOsMapMemory(psrc, nbytes + (pdst - psrc));
		vdst = vsrc + (pdst - psrc);
		memmove(vdst, vsrc, nbytes);
		AcpiOsUnmapMemory(vsrc, nbytes + (pdst - psrc));
	} else {
		/*
		 * No overlap.
		 */
		vdst = AcpiOsMapMemory(pdst, nbytes);
		vsrc = AcpiOsMapMemory(psrc, nbytes);
		memcpy(vdst, vsrc, nbytes);
		AcpiOsUnmapMemory(vsrc, nbytes);
		AcpiOsUnmapMemory(vdst, nbytes);
	}
}

/*
 * apei_erst_attach(sc)
 *
 *	Scan the Error Record Serialization Table to collate the
 *	instructions for each ERST action.
 */
void
apei_erst_attach(struct apei_softc *sc)
{
	ACPI_TABLE_ERST *erst = sc->sc_tab.erst;
	struct apei_erst_softc *ssc = &sc->sc_erst;
	ACPI_ERST_ENTRY *entry;
	uint32_t i, nentries, maxnentries;

	/*
	 * Verify the table length, table header length, and
	 * instruction entry count are all sensible.  If the header is
	 * truncated, stop here; if the entries are truncated, stop at
	 * the largest integral number of full entries that fits.
	 */
	if (erst->Header.Length < sizeof(*erst)) {
		aprint_error_dev(sc->sc_dev, "ERST: truncated table:"
		    " %"PRIu32" < %zu minimum bytes\n",
		    erst->Header.Length, sizeof(*erst));
		return;
	}
	if (erst->HeaderLength <
	    sizeof(*erst) - offsetof(ACPI_TABLE_ERST, HeaderLength)) {
		aprint_error_dev(sc->sc_dev, "ERST: truncated header:"
		    " %"PRIu32" < %zu bytes\n",
		    erst->HeaderLength,
		    sizeof(*erst) - offsetof(ACPI_TABLE_ERST, HeaderLength));
		return;
	}
	nentries = erst->Entries;
	maxnentries = (erst->Header.Length - sizeof(*erst))/sizeof(*entry);
	if (nentries > maxnentries) {
		aprint_error_dev(sc->sc_dev, "ERST: excessive entries:"
		    " %"PRIu32", truncating to %"PRIu32"\n",
		    nentries, maxnentries);
		nentries = maxnentries;
	}
	if (nentries*sizeof(*entry) < erst->Header.Length - sizeof(*erst)) {
		aprint_error_dev(sc->sc_dev, "ERST:"
		    " %zu bytes of trailing garbage after last entry\n",
		    erst->Header.Length - nentries*sizeof(*entry));
	}

	/*
	 * Create an interpreter for ERST actions.
	 */
	ssc->ssc_interp = apei_interp_create("ERST",
	    apei_erst_action, __arraycount(apei_erst_action),
	    apei_erst_instruction, __arraycount(apei_erst_instruction),
	    apei_erst_instreg, apei_erst_instvalid, apei_erst_instfunc);

	/*
	 * Compile the interpreter from the ERST action instruction
	 * table.
	 */
	entry = (ACPI_ERST_ENTRY *)(erst + 1);
	for (i = 0; i < nentries; i++, entry++)
		apei_interp_pass1_load(ssc->ssc_interp, i, &entry->WheaHeader);
	entry = (ACPI_ERST_ENTRY *)(erst + 1);
	for (i = 0; i < nentries; i++, entry++) {
		apei_interp_pass2_verify(ssc->ssc_interp, i,
		    &entry->WheaHeader);
	}
	apei_interp_pass3_alloc(ssc->ssc_interp);
	entry = (ACPI_ERST_ENTRY *)(erst + 1);
	for (i = 0; i < nentries; i++, entry++) {
		apei_interp_pass4_assemble(ssc->ssc_interp, i,
		    &entry->WheaHeader);
	}
	apei_interp_pass5_verify(ssc->ssc_interp);

	/*
	 * Print some basic information about the stored records.
	 */
	uint64_t logaddr = apei_erst_act(sc, ACPI_ERST_GET_ERROR_RANGE, 0);
	uint64_t logbytes = apei_erst_act(sc, ACPI_ERST_GET_ERROR_LENGTH, 0);
	uint64_t attr = apei_erst_act(sc, ACPI_ERST_GET_ERROR_ATTRIBUTES, 0);
	uint64_t nrecords = apei_erst_act(sc, ACPI_ERST_GET_RECORD_COUNT, 0);
	char attrbuf[128];

	/* XXX define this format somewhere */
	snprintb(attrbuf, sizeof(attrbuf), "\177\020"
	    "\001"	"NVRAM\0"
	    "\002"	"SLOW\0"
	    "\0", attr);

	aprint_normal_dev(sc->sc_dev, "ERST: %"PRIu64" records in error log"
	    " %"PRIu64" bytes @ 0x%"PRIx64" attr=%s\n",
	    nrecords, logbytes, logaddr, attrbuf);

	/*
	 * XXX wire up to sysctl or a file system or something, and/or
	 * dmesg or crash dumps
	 */
}

/*
 * apei_erst_detach(sc)
 *
 *	Free software resource allocated for ERST handling.
 */
void
apei_erst_detach(struct apei_softc *sc)
{
	struct apei_erst_softc *ssc = &sc->sc_erst;

	if (ssc->ssc_interp) {
		apei_interp_destroy(ssc->ssc_interp);
		ssc->ssc_interp = NULL;
	}
}

/*
 * apei_erst_instvalid(header, ninst, i)
 *
 *	Routine to validate the ith entry, for an action with ninst
 *	instructions.
 */
static bool
apei_erst_instvalid(ACPI_WHEA_HEADER *header, uint32_t ninst, uint32_t i)
{

	switch (header->Instruction) {
	case ACPI_ERST_GOTO:
		if (header->Value > ninst) {
			aprint_error("ERST[%"PRIu32"]:"
			    " GOTO(%"PRIu64") out of bounds,"
			    " disabling action %"PRIu32" (%s)\n", i,
			    header->Value,
			    header->Action,
			    apei_erst_action[header->Action]);
			return false;
		}
	}
	return true;
}

/*
 * struct apei_erst_machine
 *
 *	Machine state for executing ERST instructions.
 */
struct apei_erst_machine {
	struct apei_softc	*sc;
	uint64_t		x;	/* in */
	uint64_t		y;	/* out */
	uint64_t		var1;
	uint64_t		var2;
	uint64_t		src_base;
	uint64_t		dst_base;
};

/*
 * apei_erst_instfunc(header, map, cookie, &ip, maxip)
 *
 *	Run a single instruction in the service of performing an ERST
 *	action.  Updates the ERST machine at cookie, and the ip if
 *	necessary, in place.
 *
 *	On entry, ip points to the next instruction after this one
 *	sequentially; on exit, ip points to the next instruction to
 *	execute.
 */
static void
apei_erst_instfunc(ACPI_WHEA_HEADER *header, struct apei_mapreg *map,
    void *cookie, uint32_t *ipp, uint32_t maxip)
{
	struct apei_erst_machine *const M = cookie;

	/*
	 * Abbreviate some of the intermediate quantities to make the
	 * instruction logic conciser and more legible.
	 */
	const uint8_t BitOffset = header->RegisterRegion.BitOffset;
	const uint64_t Mask = header->Mask;
	const uint64_t Value = header->Value;
	ACPI_GENERIC_ADDRESS *const reg = &header->RegisterRegion;
	const bool preserve_register = header->Flags & ACPI_ERST_PRESERVE;

	aprint_debug_dev(M->sc->sc_dev, "%s: instr=0x%02"PRIx8
	    " (%s)"
	    " Address=0x%"PRIx64
	    " BitOffset=%"PRIu8" Mask=0x%"PRIx64" Value=0x%"PRIx64
	    " Flags=0x%"PRIx8"\n",
	    __func__, header->Instruction,
	    (header->Instruction < __arraycount(apei_erst_instruction)
		? apei_erst_instruction[header->Instruction]
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
	case ACPI_ERST_READ_REGISTER:
		M->y = apei_read_register(reg, map, Mask);
		break;
	case ACPI_ERST_READ_REGISTER_VALUE: {
		uint64_t v;

		v = apei_read_register(reg, map, Mask);
		M->y = (v == Value ? 1 : 0);
		break;
	}
	case ACPI_ERST_WRITE_REGISTER:
		apei_write_register(reg, map, Mask, preserve_register, M->x);
		break;
	case ACPI_ERST_WRITE_REGISTER_VALUE:
		apei_write_register(reg, map, Mask, preserve_register, Value);
		break;
	case ACPI_ERST_NOOP:
		break;
	case ACPI_ERST_LOAD_VAR1:
		M->var1 = apei_read_register(reg, map, Mask);
		break;
	case ACPI_ERST_LOAD_VAR2:
		M->var2 = apei_read_register(reg, map, Mask);
		break;
	case ACPI_ERST_STORE_VAR1:
		apei_write_register(reg, map, Mask, preserve_register,
		    M->var1);
		break;
	case ACPI_ERST_ADD:
		M->var1 += M->var2;
		break;
	case ACPI_ERST_SUBTRACT:
		/*
		 * The specification at
		 * https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#serialization-instructions
		 * says:
		 *
		 *	0x09	SUBTRACT	Subtracts VAR1 from VAR2
		 *				and stores the result in
		 *				VAR1.
		 *
		 * So, according to the spec, this is _not_ simply
		 *
		 *	M->var1 -= M->var2;
		 */
		M->var1 = M->var2 - M->var1;
		break;
	case ACPI_ERST_ADD_VALUE: {
		uint64_t v;

		v = apei_read_register(reg, map, Mask);
		v += Value;
		apei_write_register(reg, map, Mask, preserve_register, v);
		break;
	}
	case ACPI_ERST_SUBTRACT_VALUE: {
		uint64_t v;

		v = apei_read_register(reg, map, Mask);
		v -= Value;
		apei_write_register(reg, map, Mask, preserve_register, v);
		break;
	}
	case ACPI_ERST_STALL:
		DELAY(Value);		/* XXX avoid excessive delays */
		break;
	case ACPI_ERST_STALL_WHILE_TRUE:
		for (;;) {
			if (apei_read_register(reg, map, Mask) != Value)
				break;
			DELAY(M->var1);
		}
		break;
	case ACPI_ERST_SKIP_NEXT_IF_TRUE:
		/*
		 * If reading the register yields Value, skip the next
		 * instruction -- unless that would run past the end of
		 * the instruction buffer.
		 */
		if (apei_read_register(reg, map, Mask) == Value) {
			if (*ipp < maxip)
				(*ipp)++;
		}
		break;
	case ACPI_ERST_GOTO:
		if (Value >= maxip) /* paranoia */
			*ipp = maxip;
		else
			*ipp = Value;
		break;
	case ACPI_ERST_SET_SRC_ADDRESS_BASE:
		M->src_base = apei_read_register(reg, map, Mask);
		break;
	case ACPI_ERST_SET_DST_ADDRESS_BASE:
		M->src_base = apei_read_register(reg, map, Mask);
		break;
	case ACPI_ERST_MOVE_DATA: {
		const uint64_t v = apei_read_register(reg, map, Mask);

		/*
		 * XXX This might not work in nasty contexts unless we
		 * pre-allocate a virtual page for the mapping.
		 */
		apei_pmemmove(M->dst_base + v, M->src_base + v, M->var2);
		break;
	}
	default:		/* XXX unreachable */
		break;
	}
}

/*
 * apei_erst_act(sc, action, x)
 *
 *	Perform the named ERST action with input x, by stepping through
 *	all the instructions defined for the action by the ERST, and
 *	return the output.
 */
static uint64_t
apei_erst_act(struct apei_softc *sc, enum AcpiErstActions action, uint64_t x)
{
	struct apei_erst_softc *const ssc = &sc->sc_erst;
	struct apei_erst_machine erst_machine, *const M = &erst_machine;

	aprint_debug_dev(sc->sc_dev, "%s: action=%d (%s) input=0x%"PRIx64"\n",
	    __func__,
	    action,
	    (action < __arraycount(apei_erst_action)
		? apei_erst_action[action]
		: "unknown"),
	    x);

	/*
	 * Initialize the machine to execute the action's instructions.
	 */
	memset(M, 0, sizeof(*M));
	M->sc = sc;
	M->x = x;		/* input */
	M->y = 0;		/* output */
	M->var1 = 0;
	M->var2 = 0;
	M->src_base = 0;
	M->dst_base = 0;

	/*
	 * Run the interpreter.
	 */
	apei_interpret(ssc->ssc_interp, action, M);

	/*
	 * Return the result.
	 */
	aprint_debug_dev(sc->sc_dev, "%s: output=0x%"PRIx64"\n", __func__,
	    M->y);
	return M->y;
}
