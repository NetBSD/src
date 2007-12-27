/*	$NetBSD: ibcs2_exec_elf32.c,v 1.15.2.2 2007/12/27 00:43:44 mjf Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * originally from kern/exec_ecoff.c
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
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibcs2_exec_elf32.c,v 1.15.2.2 2007/12/27 00:43:44 mjf Exp $");

#define ELFSIZE		32

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>

#include <sys/cpu.h>
#include <machine/reg.h>
#include <machine/ibcs2_machdep.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_errno.h>
#include <compat/ibcs2/ibcs2_util.h>

static int ibcs2_elf32_signature(struct lwp *l, struct exec_package *,
				      Elf32_Ehdr *);

/*
 * The SCO compiler adds the string "SCO" to the .notes section of all
 * binaries I've seen so far.
 *
 * XXX - probably should only compare the id in the actual ELF notes struct
 */

#define SCO_SIGNATURE	"\004\0\0\0\014\0\0\0\001\0\0\0SCO\0"

static int
ibcs2_elf32_signature(struct lwp *l, struct exec_package *epp, Elf32_Ehdr *eh)
{
	size_t shsize = sizeof(Elf32_Shdr) * eh->e_shnum;
	size_t i;
	static const char signature[] = SCO_SIGNATURE;
	char tbuf[sizeof(signature) - 1];
	Elf32_Shdr *sh;
	int error;

	if (shsize > 64 * 1024)
		return ENOEXEC;

	sh = (Elf32_Shdr *)malloc(shsize, M_TEMP, M_WAITOK);

	if ((error = exec_read_from(l, epp->ep_vp, eh->e_shoff, sh,
	    shsize)) != 0)
		goto out;

	for (i = 0; i < eh->e_shnum; i++) {
		Elf32_Shdr *s = &sh[i];
		if (s->sh_type != SHT_NOTE ||
		    s->sh_flags != 0 ||
		    s->sh_size < sizeof(signature) - 1)
			continue;

		if ((error = exec_read_from(l, epp->ep_vp, s->sh_offset, tbuf,
		    sizeof(signature) - 1)) != 0)
			goto out;

		if (memcmp(tbuf, signature, sizeof(signature) - 1) == 0)
			goto out;
		else
			break;	/* only one .note section so quit */
	}
	error = EFTYPE;

out:
	free(sh, M_TEMP);
	return error;
}

/*
 * ibcs2_elf32_probe - search the executable for signs of SCO
 */

int
ibcs2_elf32_probe(struct lwp *l, struct exec_package *epp, void *eh, char *itp,
    vaddr_t *pos)
{
	int error;

	if ((error = ibcs2_elf32_signature(l, epp, eh)) != 0)
                return error;

	if (itp) {
		if ((error = emul_find_interp(l, epp, itp)))
			return error;
	}
	return 0;
}
