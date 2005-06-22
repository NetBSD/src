/*	$NetBSD: devopen.h,v 1.2 2005/06/22 06:06:34 junyoung Exp $	*/

extern int boot_biosdev;

void bios2dev(int, u_int, char **, int *, int *);
