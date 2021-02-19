/*	$NetBSD: mem_p.h,v 1.3 2021/02/19 16:42:19 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_MEM_P_H
#define ISC_MEM_P_H

/*! \file */

void
isc__mem_printactive(isc_mem_t *mctx, FILE *file);
/*%<
 * For use by unit tests, prints active memory blocks for
 * a single memory context.
 */

#endif /* ISC_MEM_P_H */
