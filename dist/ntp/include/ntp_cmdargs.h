/*	$NetBSD: ntp_cmdargs.h,v 1.1.1.2 2006/06/11 14:59:24 kardel Exp $	*/

#include "ntp_types.h"

extern	void	getstartup	P((int, char **));
extern	void	getCmdOpts	P((int, char **));
extern	void	ntpd_usage	P((void));
