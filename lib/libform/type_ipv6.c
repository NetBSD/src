/*	$NetBSD: type_ipv6.c,v 1.1 2001/02/10 14:51:32 blymn Exp $	*/

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
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include "form.h"
#include "internals.h"

/*
 * The IP v6 address type handling.
 */

/*
 * Check the contents of the field buffer are a valid Ipv6 address only.
 */
static int
ipv6_check_field(FIELD *field, char *args)
{
	char *keeper, *p, *buf, *newbuf, cleaned[48];
	unsigned int has_dot, compressed, v4_mode, allow_compress;
	unsigned long vals[10];
	int i, j;

	if (args == NULL)
		return FALSE;
	
	if (asprintf(&keeper, "%s", args) < 0)
		return FALSE;

	has_dot = FALSE;
	if (index(keeper, '.') != NULL)
		has_dot = TRUE;

	compressed = FALSE;
	for (i = 0; i < 10; i++)
		vals[i] = 0;

	  /*
	   * first we forward scan through the address, filling vals
	   * with the values between the :'s, if we hit a :: we know we
	   * have a compressed set of 0's so we leave the rest for
	   * the next loop...
	   */
	buf = keeper;
	v4_mode = FALSE;
	for (i = 0; i < 10; i++) {
		if (*buf == '\0')
			goto FAIL;
		if ((*buf == ':') && (*(buf + 1) == ':')) {
			compressed = TRUE;
			break;
		}

		  /* we don't have more than 8 fields in a pure v6 address */
		if ((i > 7) && (*buf != '\0') && (has_dot == FALSE))
			goto FAIL;

		  /* check for premature ipv4 compat address */
		if ((i < 6) && (*buf == '.'))
			goto FAIL;
		
		
		if ((*buf == ':') || (*buf == '.'))
			buf++;
			
		vals[i] = strtoul(buf, &newbuf, (v4_mode == FALSE)? 16 : 10);
		if (vals[i] > ((v4_mode == FALSE)? 65535 : 255))
			goto FAIL;
		
		if ((v4_mode == FALSE) && (*newbuf == '.')) {
			v4_mode = TRUE;
			vals[i] = strtoul(buf, &newbuf, 10);
			if (vals[i] > 255)
				goto FAIL;
		}
		
		buf = newbuf;
	}

	  /*
	   * Check if we had a 0 compression in the address.  Deal
	   * with this by starting at the end of the address and
	   * working backwards filling in the vals until we find a ::
	   * this way all the 0's should just sort themselves out.
	   */
	if (compressed == TRUE) {
		buf = keeper;
		if (has_dot == TRUE) {
			  /* we have a dotted quad on the end, process it */
			i = 9;
			for (j = 0; j < 3; j++) {
				if ((p = rindex(buf, '.')) == NULL)
					goto FAIL;
				p++;
				if (!isdigit(*p))
					goto FAIL;
				vals[i] = strtoul(p, NULL, 10);
				if (vals[i] > 255)
					goto FAIL;
				i--;
				p--;
				*p = '\0';
			}

			  /* check for further dots - they are not allowed */
			if (rindex(buf, '.') != NULL)
				goto FAIL;
			
		} else
			i = 7;

		  /* now scan backwards for colons since we have handled
		   * any dots that may have been there.
		   */

		do {
			if ((p = rindex(buf, ':')) == NULL)
				goto FAIL;
			
			p++;
			if (!isxdigit(*p))
				goto FAIL;
			vals[i] = strtoul(p, NULL, 16);
			if (vals[i] > 65535)
				goto FAIL;
			p--;
			i--;
			if (i < 0)
				goto FAIL;
			if (!((*p == ':') && (*(p - 1) == ':')))
				break;
			*p = '\0';
		} while (/* CONSTCOND */ 1);
	}
	

	  /*
	   * WHEW - if we got here then we have a vals array with the
	   * converted IPV6 address in it.  Reset the field buffer to
	   * the cleaned up version.
	   */
	if (has_dot == TRUE)
		i = 6;
	else
		i = 8;

	p = cleaned;
	compressed = FALSE;
	allow_compress = TRUE;
	if (vals[0] != 0)
		p += sprintf(p, "%x", (unsigned int) vals[0]);
	else {
		allow_compress = FALSE;
		compressed = TRUE;
	}
	
	*(p++) = ':';
	*p = '\0';
	for (j = 1; j < i; j++) {
		if (vals[j] == 0) {
			  /* if we are compressing 0's just continue */
			if (compressed == TRUE)
				continue;

			  /* if we have not done a :: compress before, do it */
			if (allow_compress == TRUE) {
				compressed = TRUE;
				allow_compress = FALSE;
				*(p++) = ':';
				*p = '\0';
				continue;
			}
			  /* otherwise just drop through to add the 0's */
		} else
			compressed = FALSE;
		
		p += sprintf(p, "%x:", (unsigned int) vals[j]);
	}

	  /* if the entire v6 part of the address was 0 then add a : */
	if (compressed == TRUE) {
		*(p++) = ':';
		p = '\0';
	}
	
	  /* tack on the ipv4 part if it was there before... */
	if (has_dot == TRUE)
		sprintf(p, "%u.%u.%u.%u", (unsigned int) vals[6],
			(unsigned int) vals[7], (unsigned int) vals[8],
			(unsigned int) vals[9]);

	  /* re-set the field buffer to be the reformatted IPv6 address */
	set_field_buffer(field, 0, buf);

	free(keeper);
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
ipv6_check_char(/* ARGSUSED1 */ int c, char *args)
{
	return (isxdigit(c) || (c == '.') || (c == ':')) ? TRUE : FALSE;
}

static FIELDTYPE builtin_ipv6 = {
	_TYPE_IS_BUILTIN,                   /* flags */
	0,                                  /* refcount */
	NULL,                               /* link */
	NULL,                               /* make_args */
	NULL,                               /* copy_args */
	NULL,                               /* free_args */
	ipv6_check_field,                   /* field_check */
	ipv6_check_char,                    /* char_check */
	NULL,                               /* next_choice */
	NULL                                /* prev_choice */
};

FIELDTYPE *TYPE_IPV6 = &builtin_ipv6;


