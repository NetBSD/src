/*	$NetBSD: txtwalk.h,v 1.6 2000/12/22 10:12:13 mrg Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* txtwalk.h - definitions for textwalk to be useful! */

/* data extractions */
enum kind {INT, STR};

struct data {
	enum kind what;
	union udata {
		int   i_val;
		char *s_val;
	} u;
};

/* Strings of interest! */
struct lookfor {
	char *head;	/* Line starts this way. */
	char *fmt;	/* Expected format. */
	char *todo;	/* What to do ... */
	void *var;	/* Possible var */
	int  nument;    /* Number of entries in the "array" */
	int  size;	/* size of string variables */
	void (*func) (struct data *list, int num);	/* function to call */
};

/*  Format string for the expected string:
 *  space => skip white space
 *  %ic   => skip until you find char c, if c == space, matches any white space
 *  %d    => expect an integer
 *  %s    => expect a non-white space string
 *  \0    => ignore remainder of line
 *  %%    => match a %
 *
 */

/* What to do ....
 * 
 *  a -- assign to var, $n is data from the expect strings.  
 *  c -- call the function
 *
 */

/* prototypes */

void walk (char *, size_t, struct lookfor *, int);


/* Maximum number of matched data elements per line! */
#define MAXDATA 20

