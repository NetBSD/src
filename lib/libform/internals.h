/*	$NetBSD: internals.h,v 1.2 2001/01/04 12:30:37 blymn Exp $	*/

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

#include <stdio.h>
#include "form.h"

#ifndef FORMI_INTERNALS_H
#define FORMI_INTERNALS_H 1

#ifdef DEBUG
extern FILE *dbg;
#endif

/* direction definitions for _formi_pos_new_field */
#define _FORMI_BACKWARD 1
#define _FORMI_FORWARD  2

/* define the default options for a form... */
#define DEFAULT_FORM_OPTS (O_VISIBLE | O_ACTIVE | O_PUBLIC | O_EDIT | \
                           O_WRAP | O_BLANK | O_AUTOSKIP | O_NULLOK | \
                           O_PASSOK | O_STATIC)

/* definitions of the flags for the FIELDTYPE structure */
#define _TYPE_NO_FLAGS   0
#define _TYPE_HAS_ARGS   0x01
#define _TYPE_IS_LINKED  0x02
#define _TYPE_IS_BUILTIN 0x04
#define _TYPE_HAS_CHOICE 0x08

typedef struct formi_type_link_struct formi_type_link;

struct formi_type_link_struct
{
	FIELDTYPE *next;
	FIELDTYPE *prev;
};


struct _formi_page_struct
{
	int in_use;
	int first;
	int last;
	int top_left;
	int bottom_right;
};

/* function prototypes */
unsigned
skip_blanks(char *, unsigned int);
int
_formi_add_char(FIELD *cur, unsigned pos, char c);
int
_formi_draw_page(FORM *form);
int
_formi_find_bol(const char *string, unsigned int offset);
int
_formi_find_eol(const char *string, unsigned int offset);
int
_formi_find_pages(FORM *form);
int
_formi_field_choice(FORM *form, int c);
int
_formi_manipulate_field(FORM *form, int c);
int
_formi_pos_first_field(FORM *form);
int
_formi_pos_new_field(FORM *form, unsigned direction, unsigned use_sorted);
void
_formi_sort_fields(FORM *form);
void
_formi_stitch_fields(FORM *form);
int
_formi_update_field(FORM *form, int old_field);
int
_formi_validate_field(FORM *form);

#endif



