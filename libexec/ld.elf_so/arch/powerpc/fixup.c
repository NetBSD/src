/* $NetBSD: fixup.c,v 1.3 2026/01/18 19:19:09 jmcneill Exp $ */

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
__RCSID("$NetBSD: fixup.c,v 1.3 2026/01/18 19:19:09 jmcneill Exp $");
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

static int
_rtld_espresso_fixup_range(caddr_t data_addr, size_t data_size, int data_prot)
{
	uint32_t *start, *end, *where;
	union instr previ;

	start = (uint32_t *)data_addr;
	end = start + data_size / sizeof(*where);
	previ.i_int = 0;

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

static int
_rtld_espresso_fixup(const char *path, int fd, Elf_Ehdr *ehdr, Elf_Phdr *phdr,
    caddr_t data_addr, size_t data_size, int data_prot)
{
	Elf_Shdr *shdr;
	size_t shdr_size;
	int i;

	if (_rtld_ncpus == 1 || ehdr->e_shnum == 0 ||
	    (phdr->p_flags & PF_X) == 0) {
		return 0;
	}

	shdr_size = (size_t)ehdr->e_shentsize * ehdr->e_shnum;
	shdr = mmap(NULL, shdr_size, PROT_READ, MAP_FILE | MAP_SHARED, fd,
	    ehdr->e_shoff);
	if (shdr == MAP_FAILED) {
		_rtld_error("%s: mmap of shdr failed: %s", path,
		    xstrerror(errno));
		return -1;
	}

	for (i = 0; i < ehdr->e_shnum; i++) {
		Elf_Addr start = shdr[i].sh_addr;
		Elf_Addr end = shdr[i].sh_addr + shdr[i].sh_size - 1;

		if (shdr[i].sh_type != SHT_PROGBITS) {
			continue;
		}
		if ((shdr[i].sh_flags & SHF_EXECINSTR) == 0) {
			continue;
		}

		if (start >= phdr->p_vaddr &&
		    end < phdr->p_vaddr + phdr->p_filesz) {
			dbg(("%s: fixup (espresso) from %p to %p", path,
			    (void *)start, (void *)end));

			if (_rtld_espresso_fixup_range(
			    data_addr + (start - phdr->p_vaddr),
			    shdr[i].sh_size, data_prot) != 0) {
				_rtld_error("%s: fixup failed", path);
				munmap(shdr, shdr_size);
				return -1;
			}
		}
	}

	munmap(shdr, shdr_size);

	return 0;
}

int
_rtld_map_segment_fixup(const char *path, int fd, Elf_Ehdr *ehdr,
    Elf_Phdr *phdr, caddr_t data_addr, size_t data_size, int data_prot)
{
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

	if (IBMESPRESSO_P(_rtld_ppc_pvr)) {
		return _rtld_espresso_fixup(path, fd, ehdr, phdr, data_addr,
		    data_size, data_prot);
	}

	return 0;
}
