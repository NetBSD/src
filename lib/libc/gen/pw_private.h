/*	$NetBSD: pw_private.h,v 1.1 1998/06/27 05:08:22 thorpej Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@netbsd.org>, June 26, 1998.
 * Public domain.
 */

int	__pw_scan __P((char *bp, struct passwd *pw, int *flags));
