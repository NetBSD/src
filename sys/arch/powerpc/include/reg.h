/*	$NetBSD: reg.h,v 1.2.10.1 1999/06/21 01:01:04 thorpej Exp $	*/

struct reg {
	register_t fixreg[32];
	register_t lr;
	int cr;
	int xer;
	register_t ctr;
	register_t pc;
};
