/*	$NetBSD: pw_private.h,v 1.2.56.1 2012/04/17 00:05:19 yamt Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, June 26, 1998.
 * Public domain.
 */

int	__pw_scan(char *bp, struct passwd *pw, int *flags);
