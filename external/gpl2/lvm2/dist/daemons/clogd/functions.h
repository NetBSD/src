/*	$NetBSD: functions.h,v 1.1.1.1.2.2 2009/05/13 18:52:40 jym Exp $	*/

#ifndef __CLOG_FUNCTIONS_DOT_H__
#define __CLOG_FUNCTIONS_DOT_H__

#include <linux/dm-clog-tfr.h>

#define LOG_RESUMED   1
#define LOG_SUSPENDED 2

int local_resume(struct clog_tfr *tfr);
int cluster_postsuspend(char *);

int do_request(struct clog_tfr *tfr, int server);
int push_state(const char *uuid, const char *which, char **buf);
int pull_state(const char *uuid, const char *which, char *buf, int size);

int log_get_state(struct clog_tfr *tfr);
int log_status(void);
void log_debug(void);

#endif /* __CLOG_FUNCTIONS_DOT_H__ */
