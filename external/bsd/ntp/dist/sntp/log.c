/*	$NetBSD: log.c,v 1.7.2.2 2015/01/07 04:45:35 msaitoh Exp $	*/

#include <config.h>

#include "log.h"

char *progname;		/* for msyslog use too */

static void cleanup_log(void);

void
sntp_init_logging(
	const char *prog
	)
{
	msyslog_term = TRUE;
	init_logging(prog, 0, FALSE);
	msyslog_term_pid = FALSE;
	msyslog_include_timestamp = FALSE;
}

void
open_logfile(
	const char *logfile
	)
{
	change_logfile(logfile, FALSE);
	atexit(cleanup_log);
}


static void
cleanup_log(void)
{
	syslogit = TRUE;
	fflush(syslog_file);
	fclose(syslog_file);
	syslog_file = NULL;
}
