/*	$NetBSD: reg.h,v 1.3.2.1 2000/11/20 20:31:11 bouyer Exp $	*/

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
