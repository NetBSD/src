/*	$NetBSD: lang.h,v 1.5.2.1 2024/02/25 15:47:21 martin Exp $	*/

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

#pragma once

/*! \file isc/lang.h */

#ifdef __cplusplus
#define ISC_LANG_BEGINDECLS extern "C" {
#define ISC_LANG_ENDDECLS   }
#else /* ifdef __cplusplus */
#define ISC_LANG_BEGINDECLS
#define ISC_LANG_ENDDECLS
#endif /* ifdef __cplusplus */
