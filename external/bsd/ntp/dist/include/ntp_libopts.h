/*	$NetBSD: ntp_libopts.h,v 1.1.1.2 2013/12/27 23:30:46 christos Exp $	*/

/*
 * ntp_libopts.h
 *
 * Common code interfacing with Autogen's libopts command-line option
 * processing.
 */
#ifndef NTP_LIBOPTS_H
# define NTP_LIBOPTS_H
# include "autoopts/options.h"

extern	int	ntpOptionProcess(tOptions *pOpts, int argc,
				 char ** argv);
extern	void	ntpOptionPrintVersion(tOptions *, tOptDesc *);
#endif
