/*	$NetBSD: arrayparm.c,v 1.1.1.1 2003/10/06 15:50:50 wiz Exp $	*/

/*
 * arrayparm.c --- figure out how to make a parameter be an array
 *
 * Arnold Robbins
 * arnold@skeeve.com
 * 10/2001
 *
 * Revised 7/2003
 */

/*
 * Copyright (C) 2001, 2003 the Free Software Foundation, Inc.
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

/*  do_mkarray --- turn a variable into an array */

/*
 * From awk, call
 *
 * 	mkarray(var, sub, val)
 */

static NODE *
do_mkarray(tree)
NODE *tree;
{
	int ret = -1;
	NODE *var, *sub, *val;
	NODE **elemval;

	if  (do_lint && tree->param_cnt > 3)
		lintwarn("mkarray: called with too many arguments");

	var = get_argument(tree, 0);
	if (var == NULL)
		var = stack_ptr[0];

	var = get_array(var);
	sub = get_argument(tree, 1);
	val = get_argument(tree, 2);

	printf("var->type = %s\n", nodetype2str(var->type));
	printf("sub->type = %s\n", nodetype2str(sub->type));
	printf("val->type = %s\n", nodetype2str(val->type));

	assoc_clear(var);

	elemval = assoc_lookup(var, sub, 0);
	*elemval = dupnode(val);
	ret = 0;


	/* Set the return value */
	set_value(tmp_number((AWKNUM) ret));

	/* Just to make the interpreter happy */
	return tmp_number((AWKNUM) 0);
}

/* dlload --- load new builtins in this library */

NODE *
dlload(tree, dl)
NODE *tree;
void *dl;
{
	make_builtin("mkarray", do_mkarray, 3);

	return tmp_number((AWKNUM) 0);
}
