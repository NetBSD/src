/*	$NetBSD: ffilecopy.c,v 1.9 2008/05/30 14:19:57 christos Exp $	*/

/*
 * Copyright (c) 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator   or   Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the rights
 * to redistribute these changes.
 */

#include <stdio.h>
#include "supcdefs.h"
#include "supextern.h"

/*
 *	Define two macros for fast file copying
 *
 *	FBUF_HACK: this will copy data directly between the
 *	underlying OSs FILE buffers
 *
 *	FFLAG_HACK: this will set necessary OS FILE flags
 *	after we've hacked the buffers up
 */

#if defined(__SEOF)

# define FBUF_HACK(here,there) \
	do { \
		if ((here)->_r > 0) { \
			i = write(therefile, (here)->_p, (here)->_r); \
			if (i != (here)->_r) \
				return EOF; \
			(here)->_p = (here)->_bf._base; \
			(here)->_r = 0; \
		} \
	} while (/*CONSTCOND*/0)
# define FFLAG_HACK(file) (file)->_flags |= __SEOF

#elif defined(_IOEOF)

# define FBUF_HACK(here, there) \
	do { \
		if ((here)->_cnt > 0) { \
			i = write(therefile, (here)->_ptr, (here)->_cnt ); \
			if (i != (here)->_cnt) \
				return EOF; \
			(here)->_ptr = (here)->_base; \
			(here)->_cnt = 0; \
		} \
	} while (/*CONSTCOND*/0)
# define FFLAG_HACK(file) (file)->_flag |= _IOEOF;

#else

# define FBUF_HACK(here, there)	/* Nada */
# define FFLAG_HACK(here) (void)fseeko(here, (off_t)0, SEEK_END);

#endif

/*  ffilecopy  --  very fast buffered file copy
 *
 *  Usage:  i = ffilecopy (here,there)
 *	int i;
 *	FILE *here, *there;
 *
 *  Ffilecopy is a very fast routine to copy the rest of a buffered
 *  input file to a buffered output file.  Here and there are open
 *  buffers for reading and writing (respectively); ffilecopy
 *  performs a file-copy faster than you should expect to do it
 *  yourself.  Ffilecopy returns 0 if everything was OK; EOF if
 *  there was any error.  Normally, the input file will be left in
 *  EOF state (feof(here) will return TRUE), and the output file will be
 *  flushed (i.e. all data on the file rather in the core buffer).
 *  It is not necessary to flush the output file before ffilecopy.
 *
 *  HISTORY
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

int 
ffilecopy(FILE * here, FILE * there)
{
	int i, herefile, therefile;

	herefile = fileno(here);
	therefile = fileno(there);

	if (fflush(there) == EOF)	/* flush pending output */
		return EOF;

	/* if we know any buffer tricks, do them now */
	FBUF_HACK(here,there);

	/* copy the rest of the file directly (avoiding FILE buffers) */
	i = filecopy(herefile, therefile);	/* fast file copy */
	if (i < 0)
		return EOF;

	/* ensure we are at the end of the file */
	FFLAG_HACK(here);
	return 0;
}
