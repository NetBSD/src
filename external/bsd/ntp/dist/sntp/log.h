/*	$NetBSD: log.h,v 1.1.1.1 2009/12/13 16:57:10 kardel Exp $	*/

#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>


void log_msg(char *message, char type);
void debug_msg(char *message);

void init_log(const char *logfile);
void cleanup_log(void);

#endif
