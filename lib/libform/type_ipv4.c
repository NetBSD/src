/*	$NetBSD: type_ipv4.c,v 1.1 2000/12/17 12:04:31 blymn Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "form.h"
#include "internals.h"

/*
 * The IP v4 address type handling.
 */

/*
 * Check the contents of the field buffer are a valid IPv4 address only.
 */
static int /* ARGSUSED1 */
ipv4_check_field(FIELD *field, char *args)
{
	char *buf, *keeper, *p;
	unsigned int vals[4], i;
	
	if (asprintf(&keeper, "%s", field->buffers[0].string) < 0)
		return FALSE;

	buf = keeper;
	
	for (i = 0; i < 4; i++) {
		p = strsep(&buf, ".");
		if (*p == '\0')
			goto FAIL;
		vals[i] = atoi(p);
		if (vals[i] > 255)
			goto FAIL;
	}

	  /* check for null buffer pointer, indicates trailing garbage */
	if (buf != NULL)
		goto FAIL;

	free(keeper);
	
	if (asprintf(&buf, "%d.%d.%d.%d", vals[0], vals[1], vals[2],
		     vals[3]) < 0)
		return FALSE;

	  /* re-set the field buffer to be the reformatted IPv4 address */
	set_field_buffer(field, 0, buf);

	free(buf);
	
	return TRUE;

	  /* bail out point if we got a bad entry */
  FAIL:
	free(keeper);
	return FALSE;
	
}

/*
 * Check the given character is numeric, return TRUE if it is.
 */
static int
ipv4_check_char(/* ARGSUSED1 */ int c, char *args)
{
	return ((isdigit(c) || (c == '.')) ? TRUE : FALSE);
}

static FIELDTYPE builtin_ipv4 = {
	_TYPE_HAS_ARGS | _TYPE_IS_BUILTIN,  /* flags */
	0,                                  /* refcount */
	NULL,                               /* link */
	NULL,                               /* args */
	NULL,                               /* make_args */
	NULL,                               /* copy_args */
	NULL,                               /* free_args */
	ipv4_check_field,                   /* field_check */
	ipv4_check_char,                    /* char_check */
	NULL,                               /* next_choice */
	NULL                                /* prev_choice */
};

FIELDTYPE *TYPE_IPV4 = &builtin_ipv4;


