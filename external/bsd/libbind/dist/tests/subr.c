/*	$NetBSD: subr.c,v 1.1.1.1.2.2 2009/05/13 18:52:36 jym Exp $	*/

/*
 * Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef lint
static const char sccsid[] = "@(#)subr.c	5.24 (Berkeley) 3/2/91";
static const char rcsid[] = "Id: subr.c,v 1.3 2009/03/03 23:49:07 tbox Exp";
#endif /* not lint */

/*
 *******************************************************************************
 *
 *  subr.c --
 *
 *	Miscellaneous subroutines for the name server
 *	lookup program.
 *
 *	Copyright (c) 1985
 *	Andrew Cherenson
 *	U.C. Berkeley
 *	CS298-26  Fall 1985
 *
 *******************************************************************************
 */

#include "port_before.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port_after.h"

#include "resolv.h"
#include "res.h"


/*
 *******************************************************************************
 *
 *  StringToClass --
 *
 *	Converts a string form of a query class name to its
 *	corresponding integer value.
 *
 *******************************************************************************
 */
int
StringToClass(class, dflt, errorfile)
    char *class;
    int dflt;
    FILE *errorfile;
{
	int result, success;

	result = sym_ston(__p_class_syms, class, &success);
	if (success)
		return result;

	if (errorfile)
		fprintf(errorfile, "unknown query class: %s\n", class);
	return(dflt);
}


/*
 *******************************************************************************
 *
 *  StringToType --
 *
 *	Converts a string form of a query type name to its
 *	corresponding integer value.
 *
 *******************************************************************************
 */

int
StringToType(type, dflt, errorfile)
    char *type;
    int dflt;
    FILE *errorfile;
{
	int result, success;

	result = sym_ston(__p_type_syms, type, &success);
	if (success)
		return (result);

	if (errorfile)
		fprintf(errorfile, "unknown query type: %s\n", type);
	return (dflt);
}

/*
 * Skip over leading white space in SRC and then copy the next sequence of
 * non-whitespace characters into DEST. No more than (DEST_SIZE - 1)
 * characters are copied. DEST is always null-terminated. Returns 0 if no
 * characters could be copied into DEST. Returns the number of characters
 * in SRC that were processed (i.e. the count of characters in the leading
 * white space and the first non-whitespace sequence).
 *
 * 	int i;
 * 	char *p = "  foo bar ", *q;
 * 	char buf[100];
 *
 * 	q = p + pickString(p, buf, sizeof buff);
 * 	assert (strcmp (q, " bar ") == 0) ;
 *
 */

int
pickString(const char *src, char *dest, size_t dest_size) {
	const char *start;
	const char *end ;
	size_t sublen ;

	if (dest_size == 0 || dest == NULL || src == NULL)
		return 0;
	
	for (start = src ; isspace((unsigned char)*start) ; start++)
		/* nada */ ;

        for (end = start ; *end != '\0' && !isspace((unsigned char)*end) ; end++)
		/* nada */ ;

	sublen = end - start ;
	
	if (sublen == 0 || sublen > (dest_size - 1))
		return 0;

	strncpy (dest, start, sublen);

	dest[sublen] = '\0' ;

	return (end - src);
}




/*
 * match the string FORMAT against the string SRC. Leading whitespace in
 * FORMAT will match any amount of (including no) leading whitespace in
 * SRC. Any amount of whitespace inside FORMAT matches any non-zero amount
 * of whitespace in SRC. Value returned is 0 if match didn't occur, or the
 * amount of characters in SRC that did match 
 *
 * 	int i ;
 *
 * 	i = matchString(" a    b c", "a b c") ; 
 * 	assert (i == 5) ;
 * 	i = matchString("a b c", "  a b c");  
 * 	assert (i == 0) ;    becasue no leading white space in format
 * 	i = matchString(" a b c", " a   b     c"); 
 * 	assert(i == 12);
 * 	i = matchString("aa bb ", "aa      bb      ddd sd"); 
 * 	assert(i == 16);
 */
int
matchString (const char *format, const char *src) {
	const char *f = format;
	const char *s = src;

	if (f == NULL || s == NULL)
		goto notfound;

	if (isspace((unsigned char)*f)) {
		while (isspace((unsigned char)*f))
			f++ ;
		while (isspace((unsigned char)*s))
			s++ ;
	}
	
	while (1) {
		if (isspace((unsigned char)*f)) {
			if (!isspace((unsigned char)*s))
				goto notfound;
			while(isspace((unsigned char)*s))
				s++;
			/* any amount of whitespace in the format string
			   will match any amount of space in the source
			   string. */
			while (isspace((unsigned char)*f))
				f++;
		} else if (*f == '\0') {
			return (s - src);
		} else if (*f != *s) {
			goto notfound;
		} else {
			s++ ;
			f++ ;
		}
	}
 notfound:
	return 0 ;
}
