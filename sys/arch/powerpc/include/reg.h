/*	$NetBSD: reg.h,v 1.3.10.1 2000/06/22 17:02:40 minoura Exp $	*/

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
