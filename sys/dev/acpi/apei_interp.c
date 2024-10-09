/*	$NetBSD: apei_interp.c,v 1.4.4.2 2024/10/09 13:00:11 martin Exp $	*/

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
 * APEI action interpreter.
 *
 * APEI provides a generalized abstraction to implement the actions an
 * OS must take to inject an error, or save state in a persistent error
 * record for the next boot, since there are many different hardware
 * register interfaces for, e.g., injecting errors.
 *
 * You might think that APEI, being part of ACPI, would use the usual
 * ACPI interpreter to run ACPI methods for these actions.  You would
 * be wrong.  Alas.
 *
 * Instead, there is an entirely different little language of actions
 * that an OS must write programs in to inject errors, and an entirely
 * different little language of instructions that the interpreter for
 * the actions uses to interpret the OS's error injection program.  Got
 * it?
 *
 * The EINJ and ERST tables provide a series entries that look like:
 *
 *	+-----------------------------------------------+
 *	| Action=SET_ERROR_TYPE				|
 *	| Instruction=SKIP_NEXT_INSTRUCTION_IF_TRUE	|
 *	| Register=0x7fabcd10 [memory]			|
 *	| Value=0xdeadbeef				|
 *	+-----------------------------------------------+
 *	| Action=SET_ERROR_TYPE				|
 *	| Instruction=WRITE_REGISTER_VALUE		|
 *	| Register=0x7fabcd14 [memory]			|
 *	| Value=1					|
 *	+-----------------------------------------------+
 *	| Action=SET_ERROR_TYPE				|
 *	| Instruction=READ_REGISTER			|
 *	| Register=0x7fabcd1c [memory]			|
 *	+-----------------------------------------------+
 *	| Action=SET_ERROR_TYPE				|
 *	| Instruction=WRITE_REGISTER			|
 *	| Register=0x7fabcd20 [memory]			|
 *	+-----------------------------------------------+
 *	| Action=EXECUTE_OPERATION			|
 *	| Instruction=LOAD_VAR1				|
 *	| Register=0x7fabcf00 [memory]			|
 *	+-----------------------------------------------+
 *	| Action=SET_ERROR_TYPE				|
 *	| Instruction=WRITE_REGISTER_VALUE		|
 *	| Register=0x7fabcd24 [memory]			|
 *	| Value=42					|
 *	+-----------------------------------------------+
 *	| ...						|
 *	+-----------------------------------------------+
 *
 * The entries tell the OS, for each action the OS might want to
 * perform like BEGIN_INJECTION_OPERATION or SET_ERROR_TYPE or
 * EXECUTE_OPERATION, what instructions must be executed and in what
 * order.
 *
 * The instructions run in one of two little state machines -- there's
 * a different instruction set for EINJ and ERST -- and vary from noops
 * to reading and writing registers to arithmetic on registers to
 * conditional and unconditional branches.
 *
 * Yes, that means this little language -- the ERST language, anyway,
 * not the EINJ language -- is Turing-complete.
 *
 * This APEI interpreter first compiles the table into a contiguous
 * sequence of instructions for each action, to make execution easier,
 * since there's no requirement that the instructions for an action be
 * contiguous in the table, and the GOTO instruction relies on
 * contiguous indexing of the instructions for an action.
 *
 * This interpreter also does a little validation so the firmware
 * doesn't, e.g., GOTO somewhere in oblivion.  The validation is mainly
 * a convenience for catching mistakes in firmware, not a security
 * measure, since the OS is absolutely vulnerable to malicious firmware
 * anyway.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apei_interp.c,v 1.4.4.2 2024/10/09 13:00:11 martin Exp $");

#include <sys/types.h>

#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/apei_interp.h>
#include <dev/acpi/apei_mapreg.h>

/*
 * struct apei_actinst
 *
 *	Sequence of instructions to execute for an action.
 */
struct apei_actinst {
	uint32_t		ninst;
	uint32_t		ip;
	struct {
		struct acpi_whea_header	*header;
		struct apei_mapreg	*map;
	}			*inst;
	bool			disable;
};

/*
 * struct apei_interp
 *
 *	Table of instructions to interpret APEI actions.
*/
struct apei_interp {
	const char		*name;
	const char		*const *actname;
	unsigned		nact;
	const char		*const *instname;
	unsigned		ninst;
	const bool		*instreg;
	bool			(*instvalid)(ACPI_WHEA_HEADER *, uint32_t,
				    uint32_t);
	void			(*instfunc)(ACPI_WHEA_HEADER *,
				    struct apei_mapreg *, void *, uint32_t *,
				    uint32_t);
	struct apei_actinst	actinst[];
};

struct apei_interp *
apei_interp_create(const char *name,
    const char *const *actname, unsigned nact,
    const char *const *instname, unsigned ninst,
    const bool *instreg,
    bool (*instvalid)(ACPI_WHEA_HEADER *, uint32_t, uint32_t),
    void (*instfunc)(ACPI_WHEA_HEADER *, struct apei_mapreg *, void *,
	uint32_t *, uint32_t))
{
	struct apei_interp *I;

	I = kmem_zalloc(offsetof(struct apei_interp, actinst[nact]), KM_SLEEP);
	I->name = name;
	I->actname = actname;
	I->nact = nact;
	I->instname = instname;
	I->ninst = ninst;
	I->instreg = instreg;
	I->instvalid = instvalid;
	I->instfunc = instfunc;

	return I;
}

void
apei_interp_destroy(struct apei_interp *I)
{
	unsigned action, nact = I->nact;

	for (action = 0; action < nact; action++) {
		struct apei_actinst *const A = &I->actinst[action];
		unsigned j;

		if (A->ninst == 0 || A->inst == NULL)
			continue;

		for (j = 0; j < A->ninst; j++) {
			ACPI_WHEA_HEADER *const E = A->inst[j].header;
			struct apei_mapreg *const map = A->inst[j].map;

			if (map != NULL)
				apei_mapreg_unmap(&E->RegisterRegion, map);
		}

		kmem_free(A->inst, A->ninst * sizeof(A->inst[0]));
		A->inst = NULL;
	}

	kmem_free(I, offsetof(struct apei_interp, actinst[nact]));
}

/*
 * apei_interp_pass1_load(I, i, E)
 *
 *	Load the ith table entry E into the interpreter I.  To be
 *	called for each entry in the table sequentially.
 *
 *	This first pass counts the number of instructions for each
 *	action, so we can allocate an array of instructions for
 *	indexing each action.
 */
void
apei_interp_pass1_load(struct apei_interp *I, uint32_t i,
    ACPI_WHEA_HEADER *E)
{

	/*
	 * If we don't recognize this action, ignore it and move on.
	 */
	if (E->Action >= I->nact || I->actname[E->Action] == NULL) {
		aprint_error("%s[%"PRIu32"]: unknown action: 0x%"PRIx8"\n",
		    I->name, i, E->Action);
		return;
	}
	struct apei_actinst *const A = &I->actinst[E->Action];

	/*
	 * If we can't interpret this instruction for this action, or
	 * if we couldn't interpret a previous instruction for this
	 * action, disable this action and move on.
	 */
	if (E->Instruction >= I->ninst ||
	    I->instname[E->Instruction] == NULL) {
		aprint_error("%s[%"PRIu32"]: unknown instruction: 0x%02"PRIx8
		    "\n", I->name, i, E->Instruction);
		A->ninst = 0;
		A->disable = true;
		return;
	}
	if (A->disable)
		return;

	/*
	 * Count another instruction.  We will make a pointer
	 * to it in a later pass.
	 */
	A->ninst++;

	/*
	 * If it overflows a reasonable size, disable the action
	 * altogether.
	 */
	if (A->ninst >= 256) {
		aprint_error("%s[%"PRIu32"]:"
		    " too many instructions for action %"PRIu32" (%s)\n",
		    I->name, i,
		    E->Action, I->actname[E->Action]);
		A->ninst = 0;
		A->disable = true;
		return;
	}
}

/*
 * apei_interp_pass2_verify(I, i, E)
 *
 *	Verify the ith entry's instruction, using the caller's
 *	instvalid function, now that all the instructions have been
 *	counted.  To be called for each entry in the table
 *	sequentially.
 *
 *	This second pass checks that GOTO instructions in particular
 *	don't jump out of bounds.
 */
void
apei_interp_pass2_verify(struct apei_interp *I, uint32_t i,
    ACPI_WHEA_HEADER *E)
{

	/*
	 * If there's no instruction validation function, skip this
	 * pass.
	 */
	if (I->instvalid == NULL)
		return;

	/*
	 * If we skipped it in earlier passes, skip it now.
	 */
	if (E->Action > I->nact || I->actname[E->Action] == NULL)
		return;

	/*
	 * If the instruction is invalid, disable the whole action.
	 */
	struct apei_actinst *const A = &I->actinst[E->Action];
	if (!(*I->instvalid)(E, A->ninst, i)) {
		A->ninst = 0;
		A->disable = true;
	}
}

/*
 * apei_interp_pass3_alloc(I)
 *
 *	Allocate an array of instructions for each action that we
 *	didn't disable.
 */
void
apei_interp_pass3_alloc(struct apei_interp *I)
{
	unsigned action;

	for (action = 0; action < I->nact; action++) {
		struct apei_actinst *const A = &I->actinst[action];
		if (A->ninst == 0 || A->disable)
			continue;
		A->inst = kmem_zalloc(A->ninst * sizeof(A->inst[0]), KM_SLEEP);
	}
}

/*
 * apei_interp_pass4_assemble(I, i, E)
 *
 *	Put the instruction for the ith entry E into the instruction
 *	array for its action.  To be called for each entry in the table
 *	sequentially.
 */
void
apei_interp_pass4_assemble(struct apei_interp *I, uint32_t i,
    ACPI_WHEA_HEADER *E)
{

	/*
	 * If we skipped it in earlier passes, skip it now.
	 */
	if (E->Action >= I->nact || I->actname[E->Action] == NULL)
		return;

	struct apei_actinst *const A = &I->actinst[E->Action];
	if (A->disable)
		return;

	KASSERT(A->ip < A->ninst);
	const uint32_t ip = A->ip++;
	A->inst[ip].header = E;
	A->inst[ip].map = I->instreg[E->Instruction] ?
	    apei_mapreg_map(&E->RegisterRegion) : NULL;
}

/*
 * apei_interp_pass5_verify(I)
 *
 *	Paranoia: Verify we got all the instructions for each action,
 *	verify the actions point to their own instructions, and dump
 *	the instructions for each action, collated, with aprint_debug.
 */
void
apei_interp_pass5_verify(struct apei_interp *I)
{
	unsigned action;

	for (action = 0; action < I->nact; action++) {
		struct apei_actinst *const A = &I->actinst[action];
		unsigned j;

		/*
		 * If the action is disabled, it's all set.
		 */
		if (A->disable)
			continue;
		KASSERTMSG(A->ip == A->ninst,
		    "action %s ip=%"PRIu32" ninstruction=%"PRIu32,
		    I->actname[action], A->ip, A->ninst);

		/*
		 * XXX Dump the complete instruction table.
		 */
		for (j = 0; j < A->ninst; j++) {
			ACPI_WHEA_HEADER *const E = A->inst[j].header;

			KASSERT(E->Action == action);

			/*
			 * If we need the register and weren't able to
			 * map it, disable the action.
			 */
			if (I->instreg[E->Instruction] &&
			    A->inst[j].map == NULL) {
				A->disable = true;
				continue;
			}

			aprint_debug("%s: %s[%"PRIu32"]: %s\n",
			    I->name, I->actname[action], j,
			    I->instname[E->Instruction]);
		}
	}
}

/*
 * apei_interpret(I, action, cookie)
 *
 *	Run the instructions associated with the given action by
 *	calling the interpreter's instfunc for each one.
 *
 *	Halt when the instruction pointer runs past the end of the
 *	array, or after 1000 cycles, whichever comes first.
 */
void
apei_interpret(struct apei_interp *I, unsigned action, void *cookie)
{
	unsigned juice = 1000;
	uint32_t ip = 0;

	if (action > I->nact || I->actname[action] == NULL)
		return;
	struct apei_actinst *const A = &I->actinst[action];
	if (A->disable)
		return;

	while (ip < A->ninst && juice --> 0) {
		ACPI_WHEA_HEADER *const E = A->inst[ip].header;
		struct apei_mapreg *const map = A->inst[ip].map;

		ip++;
		(*I->instfunc)(E, map, cookie, &ip, A->ninst);
	}
}
