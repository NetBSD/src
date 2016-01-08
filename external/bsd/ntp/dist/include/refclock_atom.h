/*	$NetBSD: refclock_atom.h,v 1.1.1.6 2016/01/08 21:21:23 christos Exp $	*/

/*
 * Definitions for the atom driver and its friends
 */
#undef NANOSECOND	/* some systems define it differently */
#define NANOSECOND	1000000000 /* one second (ns) */

struct refclock_atom {
	pps_handle_t handle;
	pps_params_t pps_params;
	struct timespec ts;
};

extern	int	refclock_ppsapi(int, struct refclock_atom *);
extern	int	refclock_params(int, struct refclock_atom *);
extern	int	refclock_pps(struct peer *, struct refclock_atom *, int);
