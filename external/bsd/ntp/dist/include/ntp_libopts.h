/*	$NetBSD: ntp_libopts.h,v 1.1.1.1.2.1 2014/12/25 02:34:32 snj Exp $	*/

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
