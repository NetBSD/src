/*	$NetBSD: pw_private.h,v 1.4 2024/01/20 14:52:47 christos Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, June 26, 1998.
 * Public domain.
 */


extern const char __yp_token[];
int	__pw_scan(char *bp, struct passwd *pw, int *flags);
