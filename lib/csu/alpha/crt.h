/*	$NetBSD: crt.h,v 1.1 1998/05/06 05:32:08 ross Exp $	*/

/* Public Domain */

typedef void  (*voidvoid)(void);

extern void __noconst_i(voidvoid *) __attribute__((section(".init")));
extern void __noconst_f(voidvoid *) __attribute__((section(".fini")));
