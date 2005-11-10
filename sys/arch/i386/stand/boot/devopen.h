/*	$NetBSD: devopen.h,v 1.1.2.1 2005/11/10 13:56:53 skrll Exp $	*/

extern int boot_biosdev;

void bios2dev(int, u_int, char **, int *, int *);
