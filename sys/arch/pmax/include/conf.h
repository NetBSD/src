/*	$NetBSD: conf.h,v 1.1 1996/04/10 16:17:23 jonathan Exp $	*/


/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * This file contributed by Jonathan Stone.
 */

#define mmread mmrw
#define mmwrite mmrw
cdev_decl(mm);


cdev_decl(scc);
cdev_decl(dc);

bdev_decl(rz);
cdev_decl(rz);

bdev_decl(tz);
cdev_decl(tz);
