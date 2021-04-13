/*	$NetBSD: type_enum.c,v 1.13 2021/04/13 13:13:04 christos Exp $	*/

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
 *
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: type_enum.c,v 1.13 2021/04/13 13:13:04 christos Exp $");

#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include "form.h"
#include "internals.h"

/*
 * Prototypes.
 */
static int
trim_blanks(char *field);

/*
 * The enum type handling.
 */

typedef struct 
{
	char **choices;
	unsigned num_choices;
	bool ignore_case;
	bool exact;
} enum_args;

/*
 * Find the first non-blank character at the end of a field, return the
 * index of that character.
 */
static int
trim_blanks(char *field)
{
	int i;

	i = (int) strlen(field);
	if (i > 0)
		i--;
	else
		return 0;
	
	while ((i > 0) && isblank((unsigned char)field[i]))
		i--;
	
	return i;
}

/*
 * Create the enum arguments structure from the given args.  Return NULL
 * if the call fails, otherwise return a pointer to the structure allocated.
 */
static char *
create_enum_args(va_list *args)
{
	enum_args *new;
	char **choices;

	new = malloc(sizeof(*new));
	if (new == NULL)
		return NULL;

	new->choices = va_arg(*args, char **);
	new->ignore_case = (va_arg(*args, int)) ? TRUE : FALSE;
	new->exact = (va_arg(*args, int)) ? TRUE : FALSE;

	_formi_dbg_printf("%s: ignore_case %d, no_blanks %d\n", __func__,
	    new->ignore_case, new->exact);
	
	  /* count the choices we have */
	choices = new->choices;
	new->num_choices = 0;
	while (*choices != NULL) {
		_formi_dbg_printf("%s: choice[%u] = \'%s\'\n", __func__,
		    new->num_choices, new->choices[new->num_choices]);
		new->num_choices++;
		choices++;
	}
	_formi_dbg_printf("%s: have %u choices\n", __func__,
	    new->num_choices);

	return (void *) new;
}

/*
 * Copy the enum argument structure.
 */
static char *
copy_enum_args(char *args)
{
	enum_args *new;

	new = malloc(sizeof(*new));

	if (new != NULL)
		memcpy(new, args, sizeof(*new));

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
 * Attempt to match the string in this to the choices given.  Returns
 * TRUE if match found otherwise FALSE.
 * 
 */
static bool
match_enum(char **choices, unsigned num_choices, bool ignore_case,
	   bool exact, char *this, unsigned *match_num)
{
	unsigned i, start, end, enum_start, blen, elen, enum_end;
	bool cur_match;

	start = _formi_skip_blanks(this, 0);
	end = trim_blanks(this);

	if (end >= start)
		blen = (unsigned) (strlen(&this[start])
				   - strlen(&this[end]) + 1);
	else
		blen = 0;

	_formi_dbg_printf("%s: start %u, blen %u\n", __func__, start, blen);
	for (i = 0; i < num_choices; i++) {
		enum_start = _formi_skip_blanks(choices[i], 0);
		enum_end = trim_blanks(choices[i]);

		if (enum_end >= enum_start)
			elen = (unsigned) (strlen(&choices[i][enum_start])
				- strlen(&choices[i][enum_end]) + 1);
		else
			elen = 0;
		
		_formi_dbg_printf("%s: checking choice \'%s\'\n", __func__,
			choices[i]);
		_formi_dbg_printf("%s: enum_start %u, elen %u\n", __func__,
			enum_start, elen);
		
		  /* don't bother if we are after an exact match
		   * and the test length is not equal to the enum
		   * in question - it will never match.
		   */
		if ((exact == TRUE) && (blen != elen))
			continue;

		  /*
		   * If the test length is longer than the enum
		   * length then there is no chance of a match
		   * so we skip.
		   */
		if ((exact != TRUE) && (blen > elen))
			continue;
		
		if (ignore_case)
			cur_match = (strncasecmp(&choices[i][enum_start],
						 &this[start],
						 (size_t)blen) == 0) ?
				TRUE : FALSE;
		else
			cur_match = (strncmp(&choices[i][enum_start],
					     &this[start],
					     (size_t) blen) == 0) ?
				TRUE : FALSE;

		_formi_dbg_printf("%s: curmatch is %s\n", __func__,
			(cur_match == TRUE)? "TRUE" : "FALSE");
		
		if (cur_match == TRUE) {
			*match_num = i;
			return TRUE;
		}

	}

	_formi_dbg_printf("%s: no match found\n", __func__);
	return FALSE;
}

/*
 * Check the contents of the field buffer match one of the enum strings only.
 */
static int
enum_check_field(FIELD *field, char *args)
{
	enum_args *ta;
	unsigned match_num;

	if (args == NULL)
		return FALSE;
	
	ta = (enum_args *) (void *) field->args;
	
	if (match_enum(ta->choices, ta->num_choices, ta->ignore_case,
		       ta->exact, args, &match_num) == TRUE) {
		_formi_dbg_printf("%s: We matched, match_num %u\n", __func__,
		    match_num);
		_formi_dbg_printf("%s: buffer is \'%s\'\n", __func__,
		    ta->choices[match_num]);
		set_field_buffer(field, 0, ta->choices[match_num]);
		return TRUE;
	}

	return FALSE;
}

/*
 * Get the next enum in the list of choices.
 */
static int
next_enum(FIELD *field, char *args)
{
	enum_args *ta;
	unsigned cur_choice;

	if (args == NULL)
		return FALSE;
	
	ta = (enum_args *) (void *) field->args;

	_formi_dbg_printf("%s: attempt to match \'%s\'\n", __func__, args);

	if (match_enum(ta->choices, ta->num_choices, ta->ignore_case,
		       ta->exact, args, &cur_choice) == FALSE) {
		_formi_dbg_printf("%s: match failed\n", __func__);
		return FALSE;
	}
	
	_formi_dbg_printf("%s: cur_choice is %u\n", __func__, cur_choice);
	
	cur_choice++;
	
	if (cur_choice >= ta->num_choices)
		cur_choice = 0;

	_formi_dbg_printf("%s: cur_choice is %u on exit\n", __func__,
	    cur_choice);
	
	set_field_buffer(field, 0, ta->choices[cur_choice]);
	return TRUE;
}

/*
 * Get the previous enum in the list of choices.
 */
static int
prev_enum(FIELD *field, char *args)
{
	enum_args *ta;
	unsigned cur_choice;

	if (args == NULL)
		return FALSE;
	
	ta = (enum_args *) (void *) field->args;
	
	_formi_dbg_printf("%s: attempt to match \'%s\'\n", __func__, args);

	if (match_enum(ta->choices, ta->num_choices, ta->ignore_case,
		       ta->exact, args, &cur_choice) == FALSE) {
		_formi_dbg_printf("%s: match failed\n", __func__);
		return FALSE;
	}

	_formi_dbg_printf("%s: cur_choice is %u\n", __func__, cur_choice);
	if (cur_choice == 0)
		cur_choice = ta->num_choices - 1;
	else
		cur_choice--;
	
	_formi_dbg_printf("%s: cur_choice is %u on exit\n",
	    __func__, cur_choice);

	set_field_buffer(field, 0, ta->choices[cur_choice]);
	return TRUE;
}


static FIELDTYPE builtin_enum = {
	_TYPE_HAS_ARGS | _TYPE_IS_BUILTIN,  /* flags */
	0,                                  /* refcount */
	NULL,                               /* link */
	create_enum_args,                  /* make_args */
	copy_enum_args,                    /* copy_args */
	free_enum_args,                    /* free_args */
	enum_check_field,                  /* field_check */
	NULL,                              /* char_check */
	next_enum,                         /* next_choice */
	prev_enum                          /* prev_choice */
};

FIELDTYPE *TYPE_ENUM = &builtin_enum;


