/*	$NetBSD: stat.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_STAT_H
#define ISC_STAT_H 1

#include <sys/stat.h>

/*
 * Windows doesn't typedef this.
 */
typedef unsigned short mode_t;

/* open() under unix allows setting of read/write permissions
 * at the owner, group and other levels.  These don't exist in NT
 * We'll just map them all to the NT equivalent
 */

#define S_IREAD	_S_IREAD	/* read permission, owner */
#define S_IWRITE _S_IWRITE	/* write permission, owner */
#define S_IRUSR _S_IREAD	/* Owner read permission */
#define S_IWUSR _S_IWRITE	/* Owner write permission */
#define S_IRGRP _S_IREAD	/* Group read permission */
#define S_IWGRP _S_IWRITE	/* Group write permission */
#define S_IROTH _S_IREAD	/* Other read permission */
#define S_IWOTH _S_IWRITE	/* Other write permission */

#ifndef S_IFMT
# define S_IFMT   _S_IFMT
#endif
#ifndef S_IFDIR
# define S_IFDIR  _S_IFDIR
#endif
#ifndef S_IFCHR
# define S_IFCHR  _S_IFCHR
#endif
#ifndef S_IFREG
# define S_IFREG  _S_IFREG
#endif

#ifndef S_ISDIR
# define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
# define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif

#endif /* ISC_STAT_H */
