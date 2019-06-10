/*	$NetBSD: task_p.h,v 1.2.4.2 2019/06/10 22:04:43 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_TASK_P_H
#define ISC_TASK_P_H

/*! \file */

/*%
 * These functions allow unit tests to manipulate the processing
 * of the task queue. They are not intended as part of the public API.
 */
void
isc__taskmgr_pause(isc_taskmgr_t *taskmgr);
void
isc__taskmgr_resume(isc_taskmgr_t *taskmgr);


#endif /* ISC_TASK_P_H */
