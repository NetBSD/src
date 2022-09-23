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

#ifndef ISC_PLATFORM_H
#define ISC_PLATFORM_H 1

/*! \file */

/*****
 ***** Platform-dependent defines.
 *****/

/***
 *** Default strerror_r buffer size
 ***/

#define ISC_STRERRORSIZE 128

/***
 *** System limitations
 ***/

#include <limits.h>

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

/***
 *** Miscellaneous.
 ***/

/*
 * Defined to <gssapi.h> or <gssapi/gssapi.h> for how to include
 * the GSSAPI header.
 */
#define ISC_PLATFORM_GSSAPIHEADER <gssapi/gssapi.h>

/*
 * Defined to <gssapi_krb5.h> or <gssapi/gssapi_krb5.h> for how to
 * include the GSSAPI KRB5 header.
 */
#define ISC_PLATFORM_GSSAPI_KRB5_HEADER <gssapi/gssapi_krb5.h>

/*
 * Defined to <krb5.h> or <krb5/krb5.h> for how to include
 * the KRB5 header.
 */
#define ISC_PLATFORM_KRB5HEADER <krb5/krb5.h>

/*
 * Define if the platform has <sys/un.h>.
 */
#define ISC_PLATFORM_HAVESYSUNH 1

/*
 * Defines for the noreturn attribute.
 */
#define ISC_PLATFORM_NORETURN_PRE
#define ISC_PLATFORM_NORETURN_POST __attribute__((noreturn))

/***
 ***	Windows dll support.
 ***/

#define LIBISC_EXTERNAL_DATA
#define LIBDNS_EXTERNAL_DATA
#define LIBISCCC_EXTERNAL_DATA
#define LIBISCCFG_EXTERNAL_DATA
#define LIBNS_EXTERNAL_DATA
#define LIBBIND9_EXTERNAL_DATA
#define LIBTESTS_EXTERNAL_DATA

/*
 * Tell emacs to use C mode for this file.
 *
 * Local Variables:
 * mode: c
 * End:
 */

#endif /* ISC_PLATFORM_H */
