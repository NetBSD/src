/* $NetBSD: dp8570a.h,v 1.1 2001/05/14 18:23:00 drochner Exp $ */

struct dp8570reg {
	u_int8_t msr;
	union {
		struct {
			u_int8_t t0cr, t1cr;
			u_int8_t pfr;
			u_int8_t irr;
		} r0;
		struct {
			u_int8_t rtmr;
			u_int8_t omr;
			u_int8_t icr0, icr1;
		} r1;
	} rx;
	u_int8_t s100c, sc, minc, hc, dmc, mc, yc, jc, j100c, dwc;
	u_int8_t t0l, t0m, t1l, t1m;
	struct {
		u_int8_t s, min, h, dm, m, dw;
	} comp;
	struct {
		u_int8_t s, min, h, dm, m;
	} sav;
};
