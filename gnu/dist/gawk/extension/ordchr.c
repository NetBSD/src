/*	$NetBSD: ordchr.c,v 1.1.1.1 2003/10/06 15:50:50 wiz Exp $	*/

/*
 * ordchr.c - Builtin functions that provide ord() and chr() functions.
 *
 * Arnold Robbins
 * arnold@skeeve.com
 * 8/2001
 */

/*
 * Copyright (C) 2001 the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include "awk.h"

/*  do_ord --- return numeric value of first char of string */

static NODE *
do_ord(tree)
NODE *tree;
{
	NODE *str;
	int ret = -1;

	if  (do_lint && tree->param_cnt > 1)
		lintwarn("ord: called with too many arguments");

	str = get_argument(tree, 0);
	if (str != NULL) {
		(void) force_string(str);
		ret = str->stptr[0];
		free_temp(str);
	} else if (do_lint)
		lintwarn("ord: called with no arguments");


	/* Set the return value */
	set_value(tmp_number((AWKNUM) ret));

	/* Just to make the interpreter happy */
	return tmp_number((AWKNUM) 0);
}

/*  do_chr --- turn numeric value into a string */

static NODE *
do_chr(tree)
NODE *tree;
{
	NODE *num;
	unsigned int ret = 0;
	AWKNUM val = 0.0;
	char str[2];

	str[0] = str[1] = '\0';

	if  (do_lint && tree->param_cnt > 1)
		lintwarn("chr: called with too many arguments");

	num = get_argument(tree, 0);
	if (num != NULL) {
		val = force_number(num);
		ret = val;	/* convert to int */
		free_temp(num);
		ret &= 0xff;
		str[0] = ret;
		str[1] = '\0';
	} else if (do_lint)
		lintwarn("chr: called with no arguments");


	/* Set the return value */
	set_value(tmp_string(str, 1));

	/* Just to make the interpreter happy */
	return tmp_number((AWKNUM) 0);
}

/* dlload --- load new builtins in this library */

NODE *
dlload(tree, dl)
NODE *tree;
void *dl;
{
	make_builtin("ord", do_ord, 1);
	make_builtin("chr", do_chr, 1);

	return tmp_number((AWKNUM) 0);
}
