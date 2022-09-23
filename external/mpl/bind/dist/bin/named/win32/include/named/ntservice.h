/*	$NetBSD: ntservice.h,v 1.5 2022/09/23 12:15:22 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef NTSERVICE_H
#define NTSERVICE_H

#include <winsvc.h>

#define BIND_DISPLAY_NAME "ISC BIND"
#define BIND_SERVICE_NAME "named"

void
     ntservice_init();
void UpdateSCM(DWORD);
void
ServiceControl(DWORD dwCtrlCode);
void
ntservice_shutdown();
BOOL
ntservice_isservice();
#endif /* ifndef NTSERVICE_H */
