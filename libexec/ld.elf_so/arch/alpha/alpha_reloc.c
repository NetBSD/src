/*	$NetBSD: alpha_reloc.c,v 1.4 2002/09/05 15:38:23 mycroft Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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

#include <sys/types.h>
#include <sys/stat.h>

#include "rtld.h"
#include "debug.h"

#ifdef RTLD_DEBUG_ALPHA
#define	adbg(x)		if (dodebug) xprintf x
#else
#define	adbg(x)		/* nothing */
#endif

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	uint32_t word0;

	/*
	 * The PLTGOT on the Alpha looks like this:
	 *
	 *	PLT HEADER
	 *	.
	 *	. 32 bytes
	 *	.
	 *	PLT ENTRY #0
	 *	.
	 *	. 12 bytes
	 *	.
	 *	PLT ENTRY #1
	 *	.
	 *	. 12 bytes
	 *	.
	 *	etc.
	 *
	 * The old-format entries look like (displacements filled in
	 * by the linker):
	 *
	 *	ldah	$28, 0($31)		# 0x279f0000
	 *	lda	$28, 0($28)		# 0x239c0000
	 *	br	$31, plt0		# 0xc3e00000
	 *
	 * The new-format entries look like:
	 *
	 *	br	$28, plt0		# 0xc3800000
	 *					# 0x00000000
	 *					# 0x00000000
	 *
	 * What we do is fetch the first PLT entry and check to
	 * see the first word of it matches the first word of the
	 * old format.  If so, we use a binding routine that can
	 * handle the old format, otherwise we use a binding routine
	 * that handles the new format.
	 *
	 * Note that this is done on a per-object basis, we can mix
	 * and match shared objects build with both the old and new
	 * linker.
	 */
	word0 = *(uint32_t *)(((char *) obj->pltgot) + 32);
	if ((word0 & 0xffff0000) == 0x279f0000) {
		/* Old PLT entry format. */
		adbg(("ALPHA: object %p has old PLT format\n", obj));
		obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start_old;
		obj->pltgot[3] = (Elf_Addr) obj;
	} else {
		/* New PLT entry format. */
		adbg(("ALPHA: object %p has new PLT format\n", obj));
		obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
		obj->pltgot[3] = (Elf_Addr) obj;
	}

	__asm __volatile("imb");
}
