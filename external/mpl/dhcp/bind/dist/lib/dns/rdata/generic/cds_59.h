/*	$NetBSD: cds_59.h,v 1.1.4.2 2024/02/29 11:38:51 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef GENERIC_CDS_59_H
#define GENERIC_CDS_59_H 1

/* CDS records have the same RDATA fields as DS records. */
typedef struct dns_rdata_ds dns_rdata_cds_t;

#endif /* GENERIC_CDS_59_H */
