/*	$NetBSD: logging.h,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#ifndef __CLUSTER_LOG_LOGGING_DOT_H__
#define __CLUSTER_LOG_LOGGING_DOT_H__

#include <stdio.h>
#include <syslog.h>

#if (BITS_PER_LONG == 64)
#define PRIu64 "lu"
#define PRId64 "ld"
#define PRIo64 "lo"
#define PRIx64 "lx"
#define PRIX64 "lX"
#define SCNu64 "lu"
#define SCNd64 "ld"
#define SCNo64 "lo"
#define SCNx64 "lx"
#define SCNX64 "lX"
#else
#define PRIu64 "Lu"
#define PRId64 "Ld"
#define PRIo64 "Lo"
#define PRIx64 "Lx"
#define PRIX64 "LX"
#define SCNu64 "Lu"
#define SCNd64 "Ld"
#define SCNo64 "Lo"
#define SCNx64 "Lx"
#define SCNX64 "LX"
#endif

/* SHORT_UUID - print last 8 chars of a string */
#define SHORT_UUID(x) (strlen(x) > 8) ? ((x) + (strlen(x) - 8)) : (x)

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
