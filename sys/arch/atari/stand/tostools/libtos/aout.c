/*	$NetBSD: aout.c,v 1.4 2001/10/14 19:47:12 leo Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Leo Weppelman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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


#ifdef TOSTOOLS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <a_out.h>

#define	MALLOC(x)	malloc(x)

#else

#include <stand.h>
#include <atari_stand.h>
#include <string.h>
#include <libkern.h>
#include <sys/exec_aout.h>

#define	MALLOC(x)	alloc(x)
#endif

#include "tosdefs.h"
#include "kparamb.h"
#include "libtos.h"
#include "cread.h"


#ifdef TOSTOOLS
/*
 * Assume compiling under TOS or MINT. The page-size will always
 * be incorrect then (if it is defined anyway).
 */
#ifdef __LDPGSZ
#undef __LDPGSZ
#endif

#define __LDPGSZ	(8*1024)	/* Page size for NetBSD		*/

#endif /* TOSTOOLS */

/*
 * Load an a.out image.
 * Exit codes:
 *	-1      : Not an a.outfile
 *	 0      : OK
 *	 error# : Error during load (*errp might contain error string).
 */
int
aout_load(fd, od, errp, loadsyms)
int	fd;
osdsc_t	*od;
char	**errp;
int	loadsyms;
{
	long		textsz, stringsz;
	struct exec	ehdr;
	int		err;

	*errp = NULL;

	lseek(fd, (off_t)0, SEEK_SET);
	if (read(fd, (char *)&ehdr, sizeof(ehdr)) != sizeof(ehdr))
		return -1;

#ifdef TOSTOOLS
	if ((ehdr.a_magic & 0xffff) != NMAGIC)
		return -1;
#else
	if ((N_GETMAGIC(ehdr) != NMAGIC) && (N_GETMAGIC(ehdr) != OMAGIC))
		return -1;
#endif

	/*
	 * Extract various sizes from the kernel executable
	 */
	textsz     = (ehdr.a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1);
	od->k_esym = 0;
	od->ksize  = textsz + ehdr.a_data + ehdr.a_bss;
	od->kentry = ehdr.a_entry;

	if (loadsyms && ehdr.a_syms) {
	
	  err = 1;
	  if (lseek(fd,ehdr.a_text+ehdr.a_data+ehdr.a_syms+sizeof(ehdr),0) <= 0)
		goto error;
	  err = 2;
	  if (read(fd, (char *)&stringsz, sizeof(long)) != sizeof(long))
		goto error;
	  err = 3;
	  if (lseek(fd, sizeof(ehdr), 0) <= 0)
		goto error;
	  od->ksize += ehdr.a_syms + sizeof(long) + stringsz;
	}

	err = 4;
	if ((od->kstart = (u_char *)MALLOC(od->ksize)) == NULL)
		goto error;

	/*
	 * Read text & data, clear bss
	 */
	err = 5;
	if ((read(fd, (char *)(od->kstart), ehdr.a_text) != ehdr.a_text)
	    ||(read(fd,(char *)(od->kstart+textsz),ehdr.a_data) != ehdr.a_data))
		goto error;
	bzero(od->kstart + textsz + ehdr.a_data, ehdr.a_bss);

	/*
	 * Read symbol and string table
	 */
	if (loadsyms && ehdr.a_syms) {
	    long	*p;

	    p = (long *)((od->kstart) + textsz + ehdr.a_data + ehdr.a_bss);
	    *p++ = ehdr.a_syms;
	    err = 6;
	    if (read(fd, (char *)p, ehdr.a_syms) != ehdr.a_syms)
		goto error;
	    p = (long *)((char *)p + ehdr.a_syms);
	    err = 7;
	    if (read(fd, (char *)p, stringsz) != stringsz)
		goto error;
	    od->k_esym = (long)((char *)p-(char *)od->kstart +stringsz);
	}
	return 0;

error:
#ifdef TOSTOOLS
	{
		static char *errs[] = {
			/* 1 */ "Cannot seek to string table",
			/* 2 */ "Cannot read string-table size",
			/* 3 */ "Cannot seek back to text start",
			/* 4 */ "Cannot malloc kernel image space",
			/* 5 */ "Unable to read kernel image",
			/* 6 */ "Cannot read symbol table",
			/* 7 */ "Cannot read string table"
		};
		*errp = errs[err];
	}
#endif /* TOSTOOLS */

	return err;
}
