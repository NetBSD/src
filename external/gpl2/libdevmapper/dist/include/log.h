/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DM_LOG_H
#define _DM_LOG_H

#include "libdevmapper.h"

#define _LOG_STDERR 128 /* force things to go to stderr, even if loglevel
			   would make them go to stdout */
#define _LOG_DEBUG 7
#define _LOG_INFO 6
#define _LOG_NOTICE 5
#define _LOG_WARN 4
#define _LOG_ERR 3
#define _LOG_FATAL 2

extern dm_log_fn dm_log;

#define plog(l, x...) dm_log(l, __FILE__, __LINE__, ## x)

#define log_error(x...) plog(_LOG_ERR, x)
#define log_print(x...) plog(_LOG_WARN, x)
#define log_warn(x...) plog(_LOG_WARN | _LOG_STDERR, x)
#define log_verbose(x...) plog(_LOG_NOTICE, x)
#define log_very_verbose(x...) plog(_LOG_INFO, x)
#define log_debug(x...) plog(_LOG_DEBUG, x)

/* System call equivalents */
#define log_sys_error(x, y) \
		log_error("%s: %s failed: %s", y, x, strerror(errno))
#define log_sys_very_verbose(x, y) \
		log_info("%s: %s failed: %s", y, x, strerror(errno))
#define log_sys_debug(x, y) \
		log_debug("%s: %s failed: %s", y, x, strerror(errno))

#define stack log_debug("<backtrace>")  /* Backtrace on error */

#define return_0	do { stack; return 0; } while (0)
#define return_NULL	do { stack; return NULL; } while (0)
#define goto_out	do { stack; goto out; } while (0)
#define goto_bad	do { stack; goto bad; } while (0)

#endif
