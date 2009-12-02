/*	$NetBSD: logging.h,v 1.1.1.1 2009/12/02 00:27:06 haad Exp $	*/

/*
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __CLUSTER_LOG_LOGGING_DOT_H__
#define __CLUSTER_LOG_LOGGING_DOT_H__

#include <stdio.h>
#include <syslog.h>

/* SHORT_UUID - print last 8 chars of a string */
#define SHORT_UUID(x) (strlen(x) > 8) ? ((x) + (strlen(x) - 8)) : (x)

extern char *__rq_types_off_by_one[];
#define RQ_TYPE(x) __rq_types_off_by_one[(x) - 1]

extern int log_tabbing;
extern int log_is_open;
extern int log_membership_change;
extern int log_checkpoint;
extern int log_resend_requests;

#define LOG_OPEN(ident, option, facility) do { \
		openlog(ident, option, facility); \
		log_is_open = 1;		  \
	} while (0)

#define LOG_CLOSE(void) do { \
		log_is_open = 0; \
		closelog();	 \
	} while (0)

#define LOG_OUTPUT(level, f, arg...) do {				\
		int __i;						\
		char __buffer[16];					\
		FILE *fp = (level > LOG_NOTICE) ? stderr : stdout;	\
		if (log_is_open) {					\
			for (__i = 0; (__i < log_tabbing) && (__i < 15); __i++) \
				__buffer[__i] = '\t';			\
			__buffer[__i] = '\0';				\
			syslog(level, "%s" f "\n", __buffer, ## arg);	\
		} else {						\
			for (__i = 0; __i < log_tabbing; __i++)		\
				fprintf(fp, "\t");			\
			fprintf(fp, f "\n", ## arg);			\
		}							\
	} while (0)


#ifdef DEBUG
#define LOG_DBG(f, arg...) LOG_OUTPUT(LOG_DEBUG, f, ## arg)
#else /* DEBUG */
#define LOG_DBG(f, arg...)
#endif /* DEBUG */

#define LOG_COND(__X, f, arg...) do {\
		if (__X) { 	     \
			LOG_OUTPUT(LOG_NOTICE, f, ## arg); \
		} \
	} while (0)
#define LOG_PRINT(f, arg...) LOG_OUTPUT(LOG_NOTICE, f, ## arg)
#define LOG_ERROR(f, arg...) LOG_OUTPUT(LOG_ERR, f, ## arg)

#endif /* __CLUSTER_LOG_LOGGING_DOT_H__ */
