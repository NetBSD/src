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

/*****
 ***** Platform-dependent defines.
 *****/

#if _MSC_VER > 1400
#define HAVE_TLS 1
#define HAVE___DECLSPEC_THREAD 1
#endif

/*
 * Some compatibility cludges
 */

#if defined(_WIN32) || defined(_WIN64)
/* We are on Windows */
# define strtok_r strtok_s

#define ISC_STRERRORSIZE 128

#ifndef strtoull
#define strtoull _strtoui64
#endif

#include <stdint.h>
#if _MSC_VER < 1914
typedef uint32_t socklen_t;
#endif

#endif

#define __builtin_unreachable() __assume(0)

/*
 * Remove __attribute__ ((foo)) on Windows
 */

#define __attribute__(attribute) /* do nothing */

/*
 * Limits
 */

#ifndef NAME_MAX
#define NAME_MAX _MAX_FNAME
#endif

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

/***
 *** Network.
 ***/

#undef MSG_TRUNC

typedef uint16_t sa_family_t;

/*
 * Define if the platform has <sys/un.h>.
 */
#undef ISC_PLATFORM_HAVESYSUNH

/*
 * Defines for the noreturn attribute.
 */
#define ISC_PLATFORM_NORETURN_PRE __declspec(noreturn)
#define ISC_PLATFORM_NORETURN_POST

/*
 * Set up a macro for importing and exporting from the DLL
 */

#ifdef LIBISC_EXPORTS
#define LIBISC_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISC_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBDNS_EXPORTS
#define LIBDNS_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBDNS_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBISCCC_EXPORTS
#define LIBISCCC_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISCCC_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBISCCFG_EXPORTS
#define LIBISCCFG_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISCCFG_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBNS_EXPORTS
#define LIBNS_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBNS_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBBIND9_EXPORTS
#define LIBBIND9_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBBIND9_EXTERNAL_DATA __declspec(dllimport)
#endif

#ifdef LIBTESTS_EXPORTS
#define LIBTESTS_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBTESTS_EXTERNAL_DATA __declspec(dllimport)
#endif

#endif /* ISC_PLATFORM_H */
