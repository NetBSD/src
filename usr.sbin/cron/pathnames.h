/*
 * $Id: pathnames.h,v 1.1.1.1 1994/01/05 20:40:16 jtc Exp $
 */

#if (defined(BSD)) && (BSD >= 199103) || defined(__linux) || defined(AIX)
# include <paths.h>
#endif /*BSD*/

#ifndef CRONDIR
			/* CRONDIR is where crond(8) and crontab(1) both chdir
			 * to; SPOOL_DIR, ALLOW_FILE, DENY_FILE, and LOG_FILE
			 * are all relative to this directory.
			 */
#define CRONDIR		"/var/cron"
#endif

			/* SPOOLDIR is where the crontabs live.
			 * This directory will have its modtime updated
			 * whenever crontab(1) changes a crontab; this is
			 * the signal for crond(8) to look at each individual
			 * crontab file and reload those whose modtimes are
			 * newer than they were last time around (or which
			 * didn't exist last time around...)
			 */
#define SPOOL_DIR	"tabs"

			/* undefining these turns off their features.  note
			 * that ALLOW_FILE and DENY_FILE must both be defined
			 * in order to enable the allow/deny code.  If neither
			 * LOG_FILE or SYSLOG is defined, we don't log.  If
			 * both are defined, we log both ways.
			 */
#define	ALLOW_FILE	"allow"		/*-*/
#define DENY_FILE	"deny"		/*-*/
#define LOG_FILE	"log"		/*-*/

			/* where should the daemon stick its PID?
			 */
#ifdef _PATH_VARRUN
# define PIDDIR	_PATH_VARRUN
#else
# define PIDDIR "/etc/"
#endif
#define PIDFILE		"%scron.pid"

			/* 4.3BSD-style crontab */
#define SYSCRONTAB	"/etc/crontab"

			/* what editor to use if no EDITOR or VISUAL
			 * environment variable specified.
			 */
#if defined(_PATH_VI)
# define EDITOR _PATH_VI
#else
# define EDITOR "/usr/ucb/vi"
#endif

#ifndef _PATH_BSHELL
# define _PATH_BSHELL "/bin/sh"
#endif

#ifndef _PATH_DEFPATH
# define _PATH_DEFPATH "/usr/bin:/bin"
#endif
