/* $NetBSD: exec_coff.c,v 1.3 2000/06/14 06:49:26 cgd Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
#ifndef lint
__RCSID("$NetBSD: exec_coff.c,v 1.3 2000/06/14 06:49:26 cgd Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "extern.h"

#if defined(NLIST_COFF)
#include <sys/exec_coff.h>

#define	check(off, size)	((off < 0) || (off + size > mappedsize))
#define	BAD			do { rv = -1; goto out; } while (0)

int
check_coff(mappedfile, mappedsize)
	const char *mappedfile;
	size_t mappedsize;
{
	struct coff_exechdr *exechdrp;
	int rv;

	rv = 0;

	if (check(0, sizeof *exechdrp))
		BAD;
	exechdrp = (struct coff_exechdr *)&mappedfile[0];

	if (COFF_BADMAG(&(exechdrp->f)))
		BAD;

out:
	return (rv);
}

int
findoff_coff(mappedfile, mappedsize, vmaddr, fileoffp)
	const char *mappedfile;
	size_t mappedsize, *fileoffp;
	u_long vmaddr;
{
	struct coff_exechdr *exechdrp;
	int rv;

	rv = 0;
	exechdrp = (struct coff_exechdr *)&mappedfile[0];

#define COFF_TXTOFF_XXX(fp, ap) \
         (COFF_ROUND(COFF_HDR_SIZE + (fp)->f_nscns * \
		    sizeof(struct coff_scnhdr), \
		     COFF_SEGMENT_ALIGNMENT(fp, ap)))

#define COFF_DATOFF_XXX(fp, ap) \
        (COFF_TXTOFF_XXX(fp, ap) + (ap)->a_tsize)

	if (exechdrp->a.a_tstart <= vmaddr &&
	    vmaddr < (exechdrp->a.a_tstart + exechdrp->a.a_tsize))
		*fileoffp = vmaddr - exechdrp->a.a_tstart +
		    COFF_TXTOFF(&exechdrp->f, &(exechdrp->a));
	else if (exechdrp->a.a_dstart <= vmaddr && 
            vmaddr < (exechdrp->a.a_dstart + exechdrp->a.a_dsize))
		*fileoffp = vmaddr - exechdrp->a.a_dstart +
		    COFF_DATOFF_XXX(&exechdrp->f, &(exechdrp->a));
	else
		BAD;

out:
	return (rv);
}

#endif /* NLIST_COFF */
