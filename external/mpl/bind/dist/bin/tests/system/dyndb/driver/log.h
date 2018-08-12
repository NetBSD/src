/*	$NetBSD: log.h,v 1.2 2018/08/12 13:02:30 christos Exp $	*/

/*
 * Copyright (C) 2009--2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef _LD_LOG_H_
#define _LD_LOG_H_

#include <isc/error.h>
#include <dns/log.h>
#include <dns/result.h>

#define fatal_error(...) \
	isc_error_fatal(__FILE__, __LINE__, __VA_ARGS__)

#define log_error_r(fmt, ...) \
	log_error(fmt ": %s", ##__VA_ARGS__, dns_result_totext(result))

#define log_error(format, ...)	\
	log_write(ISC_LOG_ERROR, format, ##__VA_ARGS__)

#define log_info(format, ...)	\
	log_write(ISC_LOG_INFO, format, ##__VA_ARGS__)

void
log_write(int level, const char *format, ...) ISC_FORMAT_PRINTF(2, 3);

#endif /* !_LD_LOG_H_ */
