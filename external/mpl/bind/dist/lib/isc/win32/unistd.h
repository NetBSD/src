/*	$NetBSD: unistd.h,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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


/* None of these are defined in NT, so define them for our use */
#define O_NONBLOCK 1
#define PORT_NONBLOCK O_NONBLOCK

/*
 * fcntl() commands
 */
#define F_SETFL 0
#define F_GETFL 1
#define F_SETFD 2
#define F_GETFD 3
/*
 * Enough problems not having full fcntl() without worrying about this!
 */
#undef F_DUPFD

int fcntl(int, int, ...);

/*
 * access() related definitions for winXP
 */
#include <io.h>
#ifndef F_OK
#define	F_OK	0
#endif

#ifndef X_OK
#define	X_OK	1
#endif

#ifndef W_OK
#define W_OK 2
#endif

#ifndef R_OK
#define R_OK 4
#endif

#define access _access

#include <process.h>
