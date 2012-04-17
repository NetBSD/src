/*	$NetBSD: log.h,v 1.2.6.1 2012/04/17 00:03:50 yamt Exp $	*/

#ifndef LOG_H
#define LOG_H

#include "ntp.h"
#include "ntp_stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>

/* syslog as ntpd does, even though we are not a daemon */
#ifdef LOG_NTP
# define OPENLOG_FAC	LOG_NTP
#else
# ifndef LOG_DAEMON
#  define LOG_DAEMON	0
# endif
# define OPENLOG_FAC	LOG_DAEMON
#endif

void init_logging(void);
void open_logfile(const char *logfile);

#endif
