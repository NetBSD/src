/*	$NetBSD: smf_globals.h,v 1.2 2018/08/12 13:02:28 christos Exp $	*/

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

#ifndef NAMED_SMF_GLOBALS_H
#define NAMED_SMF_GLOBALS_H 1

#include <libscf.h>

#undef EXTERN
#undef INIT
#ifdef NAMED_MAIN
#define EXTERN
#define INIT(v) = (v)
#else
#define EXTERN extern
#define INIT(v)
#endif

EXTERN unsigned int	named_smf_got_instance	INIT(0);
EXTERN unsigned int	named_smf_chroot	INIT(0);
EXTERN unsigned int	named_smf_want_disable	INIT(0);

isc_result_t named_smf_add_message(isc_buffer_t **text);
isc_result_t named_smf_get_instance(char **name, int debug, isc_mem_t *mctx);

#undef EXTERN
#undef INIT

#endif /* NAMED_SMF_GLOBALS_H */
