/* $NetBSD: exec_aout.c,v 1.6.40.1 2009/10/04 00:20:59 snj Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: exec_aout.c,v 1.6.40.1 2009/10/04 00:20:59 snj Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "extern.h"

#if defined(NLIST_AOUT)
#include <a.out.h>

#define	check(off, size)	((off < 0) || (off + size > mappedsize))
#define	BAD			do { rv = -1; goto out; } while (0)

extern int	T_flag_specified;
extern u_long	text_start;

int
check_aout(mappedfile, mappedsize)
	const char *mappedfile;
	size_t mappedsize;
{
	const struct exec *execp;
	int rv;

	rv = 0;

	if (check(0, sizeof *execp))
		BAD;
	execp = (const struct exec *)&mappedfile[0];

	if (N_BADMAG(*execp))
		BAD;

out:
	return (rv);
}

int
findoff_aout(mappedfile, mappedsize, vmaddr, fileoffp)
	const char *mappedfile;
	size_t mappedsize, *fileoffp;
	u_long vmaddr;
{
	const struct exec *execp;
	int rv;

	rv = 0;
	execp = (const struct exec *)&mappedfile[0];

	if (N_TXTADDR(*execp) + (execp->a_entry & (N_PAGSIZ(*execp)-1)) !=
	    execp->a_entry)
		vmaddr += N_TXTADDR(*execp) +
		    (execp->a_entry & (N_PAGSIZ(*execp)-1)) - execp->a_entry;

	/* apply correction based on the supplied text start address, if any */
	if (T_flag_specified)
		vmaddr += N_TXTADDR(*execp) - text_start;

	if (N_TXTADDR(*execp) <= vmaddr &&
	    vmaddr < (N_TXTADDR(*execp) + execp->a_text))
		*fileoffp = vmaddr - N_TXTADDR(*execp) + N_TXTOFF(*execp);
	else if (N_DATADDR(*execp) <= vmaddr &&
	    vmaddr < (N_DATADDR(*execp) + execp->a_data))
		*fileoffp = vmaddr - N_DATADDR(*execp) + N_DATOFF(*execp);
	else
		BAD;

out:
	return (rv);
}

#endif /* NLIST_AOUT */
