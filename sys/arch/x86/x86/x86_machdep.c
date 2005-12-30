/*	$NetBSD: x86_machdep.c,v 1.1 2005/12/30 13:37:57 jmmv Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_machdep.c,v 1.1 2005/12/30 13:37:57 jmmv Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/bootinfo.h>

/* --------------------------------------------------------------------- */

/*
 * Main bootinfo structure.  This is filled in by the bootstrap process
 * done in locore.S based on the information passed by the boot loader.
 */
struct bootinfo bootinfo;

/* --------------------------------------------------------------------- */

/*
 * Given the type of a bootinfo entry, looks for a matching item inside
 * the bootinfo structure.  If found, returns a pointer to it (which must
 * then be casted to the appropriate bootinfo_* type); otherwise, returns
 * NULL.
 */
void *
lookup_bootinfo(int type)
{
	boolean_t found;
	int i;
	struct btinfo_common *bic;

	bic = (struct btinfo_common *)(bootinfo.bi_data);
	found = FALSE;
	for (i = 0; i < bootinfo.bi_nentries && !found; i++) {
		if (bic->type == type)
			found = TRUE;
		else
			bic = (struct btinfo_common *)
			    ((uint8_t *)bic + bic->len);
	}

	return found ? bic : NULL;
}
