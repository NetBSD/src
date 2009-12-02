/*	$NetBSD: log.h,v 1.1.1.2 2009/12/02 00:26:22 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_LOG_H
#define _LVM_LOG_H

#include <errno.h>

/*
 * printf()-style macros to use for messages:
 *
 *   log_error   - always print to stderr.
 *   log_print   - always print to stdout.  Use this instead of printf.
 *   log_verbose - print to stdout if verbose is set (-v)
 *   log_very_verbose - print to stdout if verbose is set twice (-vv)
 *   log_debug   - print to stdout if verbose is set three times (-vvv)
 *
 * In addition, messages will be logged to file or syslog if they
 * are more serious than the log level specified with the log/debug_level
 * parameter in the configuration file.  These messages get the file
 * and line number prepended.  'stack' (without arguments) can be used 
 * to log this information at debug level.
 *
 * log_sys_error and log_sys_very_verbose are for errors from system calls
 * e.g. log_sys_error("stat", filename);
 *      /dev/fd/7: stat failed: No such file or directory
 *
 */

#include <stdio.h>		/* FILE */
#include <string.h>		/* strerror() */
#include <errno.h>

#define EUNCLASSIFIED -1	/* Generic error code */

#define _LOG_STDERR 128 /* force things to go to stderr, even if loglevel
			   would make them go to stdout */
#define _LOG_DEBUG 7
#define _LOG_INFO 6
#define _LOG_NOTICE 5
#define _LOG_WARN 4
#define _LOG_ERR 3
#define _LOG_FATAL 2

#define log_debug(x...) LOG_LINE(_LOG_DEBUG, x)
#define log_info(x...) LOG_LINE(_LOG_INFO, x)
#define log_notice(x...) LOG_LINE(_LOG_NOTICE, x)
#define log_warn(x...) LOG_LINE(_LOG_WARN | _LOG_STDERR, x)
#define log_err(x...) LOG_LINE_WITH_ERRNO(_LOG_ERR, EUNCLASSIFIED, x)
#define log_fatal(x...) LOG_LINE_WITH_ERRNO(_LOG_FATAL, EUNCLASSIFIED, x)

#define stack log_debug("<backtrace>")	/* Backtrace on error */
#define log_very_verbose(args...) log_info(args)
#define log_verbose(args...) log_notice(args)
#define log_print(args...) LOG_LINE(_LOG_WARN, args)
#define log_error(args...) log_err(args)
#define log_errno(args...) LOG_LINE_WITH_ERRNO(_LOG_ERR, args)

/* System call equivalents */
#define log_sys_error(x, y) \
		log_err("%s: %s failed: %s", y, x, strerror(errno))
#define log_sys_very_verbose(x, y) \
		log_info("%s: %s failed: %s", y, x, strerror(errno))
#define log_sys_debug(x, y) \
		log_debug("%s: %s failed: %s", y, x, strerror(errno))

#define return_0	do { stack; return 0; } while (0)
#define return_NULL	do { stack; return NULL; } while (0)
#define goto_out	do { stack; goto out; } while (0)
#define goto_bad	do { stack; goto bad; } while (0)

#endif
