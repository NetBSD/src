/*	$NetBSD: devopen.h,v 1.3 2005/12/11 12:17:47 christos Exp $	*/

extern int boot_biosdev;

void bios2dev(int, u_int, char **, int *, int *);
