/*	$NetBSD: readfile.c,v 1.1.1.1 2003/10/06 15:50:50 wiz Exp $	*/

/*
 * readfile.c - Read an entire file into a string.
 *
 * Arnold Robbins
 * Tue Apr 23 17:43:30 IDT 2002
 * Revised per Peter Tillier
 * Mon Jun  9 17:05:11 IDT 2003
 */

/*
 * Copyright (C) 2002, 2003 the Free Software Foundation, Inc.
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
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* do_readfile --- read a file into memory */

NODE *
do_readfile(tree)
NODE *tree;
{
	NODE *filename;
	int ret = -1;
	struct stat sbuf;
	char *text;
	int fd;

	if  (do_lint && tree->param_cnt > 1)
		lintwarn("readfile: called with too many arguments");

	filename = get_argument(tree, 0);
	if (filename != NULL) {
		(void) force_string(filename);

		ret = stat(filename->stptr, & sbuf);
		if (ret < 0) {
			update_ERRNO();
			free_temp(filename);
			goto done;
		} else if ((sbuf.st_mode & S_IFMT) != S_IFREG) {
			errno = EINVAL;
			ret = -1;
			update_ERRNO();
			free_temp(filename);
			goto done;
		}

		if ((fd = open(filename->stptr, O_RDONLY|O_BINARY)) < 0) {
			ret = -1;
			update_ERRNO();
			free_temp(filename);
			goto done;
		}

		emalloc(text, char *, sbuf.st_size + 2, "do_readfile");
		memset(text, '\0', sbuf.st_size + 2);

		if ((ret = read(fd, text, sbuf.st_size)) != sbuf.st_size) {
			(void) close(fd);
			ret = -1;
			update_ERRNO();
			free_temp(filename);
			goto done;
		}

		close(fd);
		free_temp(filename);
		set_value(tmp_string(text, sbuf.st_size));
		return tmp_number((AWKNUM) 0);
	} else if (do_lint)
		lintwarn("filename: called with no arguments");


done:
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
	make_builtin("readfile", do_readfile, 1);

	return tmp_number((AWKNUM) 0);
}
