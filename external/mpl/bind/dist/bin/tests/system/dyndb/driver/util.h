/*	$NetBSD: util.h,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Memory allocation and error handling utilities.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef _LD_UTIL_H_
#define _LD_UTIL_H_

#include <isc/mem.h>
#include <dns/types.h>

#include "log.h"

#define CLEANUP_WITH(result_code)				\
	do {							\
		result = (result_code);				\
		goto cleanup;					\
	} while(0)

#define CHECK(op)						\
	do {							\
		result = (op);					\
		if (result != ISC_R_SUCCESS)			\
			goto cleanup;				\
	} while (0)

#define CHECKED_MEM_GET(m, target_ptr, s)			\
	do {							\
		(target_ptr) = isc_mem_get((m), (s));		\
		if ((target_ptr) == NULL) {			\
			result = ISC_R_NOMEMORY;		\
			log_error("Memory allocation failed");	\
			goto cleanup;				\
		}						\
	} while (0)

#define CHECKED_MEM_GET_PTR(m, target_ptr)			\
	CHECKED_MEM_GET(m, target_ptr, sizeof(*(target_ptr)))

#define CHECKED_MEM_STRDUP(m, source, target)			\
	do {							\
		(target) = isc_mem_strdup((m), (source));	\
		if ((target) == NULL) {				\
			result = ISC_R_NOMEMORY;		\
			log_error("Memory allocation failed");	\
			goto cleanup;				\
		}						\
	} while (0)

#define ZERO_PTR(ptr) memset((ptr), 0, sizeof(*(ptr)))

#define MEM_PUT_AND_DETACH(target_ptr)				\
	isc_mem_putanddetach(&(target_ptr)->mctx, target_ptr,	\
			     sizeof(*(target_ptr)))

#endif /* !_LD_UTIL_H_ */
