/*	$NetBSD: log.h,v 1.2 2010/12/04 23:08:36 christos Exp $	*/

#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>


void log_msg(const char *message, char type);
void debug_msg(const char *message);

void init_log(const char *logfile);
void cleanup_log(void);

#endif
