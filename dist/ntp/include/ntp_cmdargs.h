/*	$NetBSD: ntp_cmdargs.h,v 1.3 2006/06/11 19:34:09 kardel Exp $	*/

#include "ntp_types.h"

extern	void	getstartup	P((int, char **));
extern	void	getCmdOpts	P((int, char **));
extern	void	ntpd_usage	P((void));
