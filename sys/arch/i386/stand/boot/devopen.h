/*	$NetBSD: devopen.h,v 1.3.92.1 2011/01/10 00:37:32 jym Exp $	*/

extern int boot_biosdev;

void bios2dev(int, daddr_t, char **, int *, int *);
