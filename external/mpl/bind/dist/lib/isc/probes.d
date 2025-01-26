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

provider libisc {
	probe job_cb_after(void *, void *, void *);
	probe job_cb_before(void *, void *, void *);

	probe rwlock_destroy(void *);
	probe rwlock_downgrade(void *);
	probe rwlock_init(void *);
	probe rwlock_rdlock_acq(void *);
	probe rwlock_rdlock_req(void *);
	probe rwlock_rdunlock(void *);
	probe rwlock_tryrdlock(void *, int);
	probe rwlock_tryupgrade(void *, int);
	probe rwlock_trywrlock(void *, int);
	probe rwlock_wrlock_acq(void *);
	probe rwlock_wrlock_req(void *);
	probe rwlock_wrunlock(void *);
};
