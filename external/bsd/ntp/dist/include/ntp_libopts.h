/*	$NetBSD: ntp_libopts.h,v 1.1.1.1.4.2 2012/04/17 00:03:44 yamt Exp $	*/

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
#endif
