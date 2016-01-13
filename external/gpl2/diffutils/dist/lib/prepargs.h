/*	$NetBSD: prepargs.h,v 1.1.1.1 2016/01/13 03:15:30 christos Exp $	*/

/* Parse arguments from a string and prepend them to an argv.  */

void prepend_default_options (char const *, int *, char ***);
