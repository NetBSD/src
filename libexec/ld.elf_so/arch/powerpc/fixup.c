/* $NetBSD: fixup.c,v 1.2 2026/01/15 22:13:32 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fixup.c,v 1.2 2026/01/15 22:13:32 jmcneill Exp $");
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#include <errno.h>
#include "debug.h"
#include "rtld.h"

static bool _rtld_fixup_init;
static uint32_t _rtld_ppc_pvr;
static int _rtld_ncpus;

union instr {
	u_int	i_int;
	struct {
		u_int	i_opcd:6;
		u_int	i_rs:5;
		u_int	i_ra:5;
		u_int	i_rb:5;
		u_int	i_xo:10;
		u_int	i_rc:1;
	} i_x;
};

#define OPC_integer_31		0x1f
#define OPC31_DCBST		0x036
#define OPC31_STWCX		0x096

#define IBMESPRESSO_P(_pvr)	(((_pvr) >> 16) == 0x7001)

int
_rtld_map_segment_fixup(Elf_Phdr *phdr, caddr_t data_addr, size_t data_size,
    int data_prot)
{
	uint32_t *start, *where, *end;
	union instr previ;

	if (!_rtld_fixup_init) {
		ssize_t i;
		size_t j;

		j = sizeof(_rtld_ppc_pvr);
		i = _rtld_sysctl("machdep.pvr", &_rtld_ppc_pvr, &j);
		if (i != CTLTYPE_INT) {
			_rtld_ppc_pvr = 0;
		}
		j = sizeof(_rtld_ncpus);
		i = _rtld_sysctl("hw.ncpu", &_rtld_ncpus, &j);
		if (i != CTLTYPE_INT) {
			_rtld_ncpus = 1;
		}

		_rtld_fixup_init = true;
	}
	if (!IBMESPRESSO_P(_rtld_ppc_pvr) || _rtld_ncpus == 1) {
		return 0;
	}
	if ((phdr->p_flags & PF_X) == 0) {
		return 0;
	}

	start = (uint32_t *)data_addr;
	end = start + data_size / sizeof(*where);
	previ.i_int = 0;

	dbg(("fixup (espresso) from %p to %p\n", start, end));

	if ((data_prot & PROT_WRITE) == 0 &&
	    mprotect(start, data_size, data_prot | PROT_WRITE) == -1) {
		_rtld_error("Cannot write-enable segment: %s",
		    xstrerror(errno));
		return -1;
	}

	for (where = start; where < end; where++) {
		union instr i = *(union instr *)where;

		if (i.i_x.i_opcd == OPC_integer_31 &&
		    i.i_x.i_xo == OPC31_STWCX &&
		    i.i_x.i_rc == 1) {

			if (previ.i_x.i_opcd == OPC_integer_31 &&
			    previ.i_x.i_xo == OPC31_DCBST &&
			    previ.i_x.i_rs == 0 &&
			    previ.i_x.i_ra == i.i_x.i_ra &&
			    previ.i_x.i_rb == i.i_x.i_rb) {
				dbg(("skip instruction at %p (not required)",
				    where));
				goto next_opcode;
			}

			dbg(("fixup instruction at %p: 0x%x", where, i.i_int));

			i.i_x.i_rc = 0;

			*where = i.i_int;
			__syncicache(where, 4);
		}

next_opcode:
		previ = i;
	}

	if ((data_prot & PROT_WRITE) == 0 &&
	    mprotect(start, data_size, data_prot) == -1) {
		_rtld_error("Cannot write-protect segment: %s",
		    xstrerror(errno));
		return -1;
	}

	return 0;
}
