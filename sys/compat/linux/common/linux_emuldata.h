/*	$NetBSD: linux_emuldata.h,v 1.1 1998/12/08 21:00:11 erh Exp $	*/

#ifndef _COMMON_LINUX_EMULDATA_H
#define _COMMON_LINUX_EMULDATA_H

/*
 * This is auxillary data the linux compat code
 * needs to do its work.  A pointer to it is
 * stored in the emuldata field of the proc
 * structure. (NOTYET)
 */
struct linux_emuldata {
    sigset_t	ps_siginfo;		/* Which signals have a RT handler */
};
#endif /* !_COMMON_LINUX_EMULDATA_H */
