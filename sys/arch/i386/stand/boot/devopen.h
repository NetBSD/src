/*	$NetBSD: devopen.h,v 1.5 2019/08/18 02:18:24 manu Exp $	*/

extern int boot_biosdev;

void bios2dev(int, daddr_t, char **, int *, int *, const char **);
