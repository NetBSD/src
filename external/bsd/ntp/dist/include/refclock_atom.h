/*	$NetBSD: refclock_atom.h,v 1.2 2009/12/14 00:38:20 christos Exp $	*/

/*
 * Definitions for the atom driver and its friends
 */
#ifndef NANOSECOND
#define NANOSECOND	1000000000 /* one second (ns) */
#endif
#define RANGEGATE	500000  /* range gate (ns) */

struct refclock_atom {
	pps_handle_t handle;
	pps_params_t pps_params;
	struct timespec ts;
};

extern	int	refclock_ppsapi(int, struct refclock_atom *);
extern	int	refclock_params(int, struct refclock_atom *);
extern	int	refclock_pps(struct peer *, struct refclock_atom *, int);
