/*	$NetBSD: prepargs.h,v 1.1.1.1 2003/01/26 00:43:14 wiz Exp $	*/

/* Parse arguments from a string and prepend them to an argv.  */

void prepend_default_options (char const *, int *, char ***);
