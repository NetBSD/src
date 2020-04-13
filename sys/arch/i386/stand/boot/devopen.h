/*	$NetBSD: devopen.h,v 1.4.60.1 2020/04/13 08:03:54 martin Exp $	*/

extern int boot_biosdev;

void bios2dev(int, daddr_t, char **, int *, int *, const char **);
