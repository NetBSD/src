/*	$NetBSD: reg.h,v 1.4 2000/06/04 09:30:44 tsubai Exp $	*/

struct reg {
	register_t fixreg[32];
	register_t lr;
	int cr;
	int xer;
	register_t ctr;
	register_t pc;
};

struct fpreg {
	double fpreg[32];
	double fpscr;
};
