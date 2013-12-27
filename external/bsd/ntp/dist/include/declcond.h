/*	$NetBSD: declcond.h,v 1.1.1.1 2013/12/27 23:30:46 christos Exp $	*/

/*
 * declcond.h - declarations conditionalized for ntpd
 *
 * The NTP reference implementation distribution includes two distinct
 * declcond.h files, one in ntpd/ used only by ntpd, and another in
 * include/ used by libntp and utilities.  This relies on the source
 * file's directory being ahead of include/ in the include search.
 *
 * The ntpd variant of declcond.h declares "debug" only #ifdef DEBUG,
 * as the --disable-debugging version of ntpd should not reference
 * "debug".  The libntp and utilities variant always declares debug,
 * as it is used in those codebases even without DEBUG defined.
 */
#ifndef DECLCOND_H
#define DECLCOND_H

/* #ifdef DEBUG */		/* uncommented in ntpd/declcond.h */
extern int debug;
/* #endif */			/* uncommented in ntpd/declcond.h */

#endif	/* DECLCOND_H */
