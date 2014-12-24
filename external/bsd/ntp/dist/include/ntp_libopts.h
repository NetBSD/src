/*	$NetBSD: ntp_libopts.h,v 1.1.1.2.4.1 2014/12/24 00:05:16 riz Exp $	*/

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
