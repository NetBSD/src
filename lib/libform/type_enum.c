/*	$NetBSD: type_enum.c,v 1.1 2000/12/17 12:04:31 blymn Exp $	*/

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

#include <stdlib.h>
#include <strings.h>
#include "form.h"
#include "internals.h"

/*
 * The enum type handling.
 */

typedef struct 
{
	char **choices;
	unsigned num_choices;
	bool ignore_case;
	bool no_blanks;
	unsigned cur_choice;  /* XXX this is per instance state for the
				 type.  In ncurses extraordinary lengths
				 are taken in the next choice & previous
				 choice to infer the state from the buffer
				 contents.  I am not sure if that is
				 really necessary or not.... */
} enum_args;

/*
 * Create the enum arguments structure from the given args.  Return NULL
 * if the call fails, otherwise return a pointer to the structure allocated.
 */
static char *
create_enum_args(va_list *args)
{
	enum_args *new;

	new = (enum_args *) malloc(sizeof(enum_args));

	if (new != NULL) {
		new->choices = va_arg(*args, char **);
		new->num_choices = va_arg(*args, unsigned);
		new->ignore_case = (va_arg(*args, int)) ? TRUE : FALSE;
		new->no_blanks = (va_arg(*args, int)) ? TRUE : FALSE;
		new->cur_choice = 0;
	}

	return (void *) new;
}

/*
 * Copy the the enum argument structure.
 */
static char *
copy_enum_args(char *args)
{
	enum_args *new;

	new = (enum_args *) malloc(sizeof(enum_args));

	if (new != NULL)
		bcopy(args, new, sizeof(enum_args));

	new->cur_choice = 0;
	return (void *) new;
}

/*
 * Free the allocated storage associated with the type arguments.
 */
static void
free_enum_args(char *args)
{
	if (args != NULL)
		free(args);
}

/*
 * Check the contents of the field buffer match one of the enum strings only.
 */
static int
enum_check_field(FIELD *field, char *args)
{
	char **choices, *cur;
	unsigned num_choices, i, start, enum_start, blen, elen, match_num;
	unsigned trailing;
	bool ignore_case, no_blanks, matched, cur_match;

	choices = ((enum_args *) (void *) args)->choices;
	num_choices = ((enum_args *) (void *) args)->num_choices;
	ignore_case = ((enum_args *) (void *) args)->ignore_case;
	no_blanks = ((enum_args *) (void *) args)->no_blanks;
	cur = field->buffers[0].string;

	start = skip_blanks(cur, 0);
	blen = strlen(&cur[start]);
	matched = FALSE;
	
	for (i = 0; i < num_choices; i++) {
		enum_start = skip_blanks(choices[i], 0);
		elen = strlen(&choices[i][enum_start]);

		  /* don't bother if blanks are significant and the
		   * lengths don't match - no chance of a hit.
		   */
		if (no_blanks && (blen > elen))
			continue;

		if (ignore_case)
			cur_match = (strncasecmp(&choices[i][enum_start],
						 &cur[start], elen) == 0) ?
				TRUE : FALSE;
		else
			cur_match = (strncmp(&choices[i][enum_start],
					     &cur[start], elen)) ? TRUE : FALSE;

		  /* if trailing blanks not allowed and we matched
		   * and the buffer & enum element are the same size
		   * then we have a match
		   */
		if (no_blanks && cur_match && (elen == blen)) {
			match_num = i;
			matched = TRUE;
			break;
		}

		  /*
		   * If trailing blanks allowed and we matched then check
		   * we only have trailing blanks, match if this is true.
		   * Note that we continue on here to see if there is a
		   * better match....
		   */
		if (!no_blanks && cur_match) {
			trailing = skip_blanks(cur, start + blen);
			if (cur[trailing] == '\0') {
				matched = TRUE;
				match_num = i;
				break;
			}
		}
	}

	if (matched) {
		((enum_args *) (void *) args)->cur_choice = match_num;
		set_field_buffer(field, 0, &cur[start]);
		return TRUE;
	}

	return FALSE;
}

/*
 * Get the next enum in the list of choices.
 */
static int
next_enum(/* ARGSUSED */ FIELD *field, char *args)
{
	((enum_args *) (void *) args)->cur_choice++;
	
	if (((enum_args *) (void *) args)->cur_choice
	    >= ((enum_args *) (void *) args)->num_choices)
		((enum_args *) (void *) args)->cur_choice = 0;

	return TRUE;
}

/*
 * Get the previous enum in the list of choices.
 */
static int
prev_enum(/* ARGSUSED */ FIELD *field, char *args)
{
	if (((enum_args *) (void *) args)->cur_choice == 0)
		((enum_args *) (void *) args)->cur_choice =
			((enum_args *) (void *) args)->num_choices;
	else
		((enum_args *) (void *) args)->cur_choice--;
	
	return TRUE;
}


static FIELDTYPE builtin_enum = {
	_TYPE_HAS_ARGS | _TYPE_IS_BUILTIN,  /* flags */
	0,                                  /* refcount */
	NULL,                               /* link */
	NULL,                               /* args */
	create_enum_args,                  /* make_args */
	copy_enum_args,                    /* copy_args */
	free_enum_args,                    /* free_args */
	enum_check_field,                  /* field_check */
	NULL,                              /* char_check */
	next_enum,                         /* next_choice */
	prev_enum                          /* prev_choice */
};

FIELDTYPE *TYPE_ENUM = &builtin_enum;


