/*	$NetBSD: reg.h,v 1.3 1999/05/03 10:02:19 tsubai Exp $	*/

struct reg {
	register_t fixreg[32];
	register_t lr;
	int cr;
	int xer;
	register_t ctr;
	register_t pc;
};
