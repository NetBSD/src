/*	$NetBSD: lock.h,v 1.2 2018/08/12 13:02:30 christos Exp $	*/

/*
 * Copyright (C) 2014-2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef LOCK_H_
#define LOCK_H_

#include "instance.h"
#include "util.h"

void
run_exclusive_enter(sample_instance_t *inst, isc_result_t *statep);

void
run_exclusive_exit(sample_instance_t *inst, isc_result_t state);

#endif /* LOCK_H_ */
