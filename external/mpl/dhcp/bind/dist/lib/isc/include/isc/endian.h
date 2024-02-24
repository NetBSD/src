/*	$NetBSD: endian.h,v 1.1.2.2 2024/02/24 13:07:24 martin Exp $	*/

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

#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
	defined(__OpenBSD__) || defined(__bsdi__)

#include <sys/endian.h>

/*
 * Recent BSDs should have [bl]e{16,32,64}toh() defined in <sys/endian.h>.
 * Older ones might not, but these should have the alternatively named
 * [bl]etoh{16,32,64}() functions defined.
 */
#ifndef be16toh
#define be16toh(x) betoh16(x)
#define le16toh(x) letoh16(x)
#define be32toh(x) betoh32(x)
#define le32toh(x) letoh32(x)
#define be64toh(x) betoh64(x)
#define le64toh(x) letoh64(x)
#endif /* !be16toh */

#elif defined(_WIN32)

/*
 * Windows is always little-endian and has its own byte-swapping routines, so
 * use these.
 */

#include <stdlib.h>

#define htobe16(x) _byteswap_ushort(x)
#define htole16(x) (x)
#define be16toh(x) _byteswap_ushort(x)
#define le16toh(x) (x)

#define htobe32(x) _byteswap_ulong(x)
#define htole32(x) (x)
#define be32toh(x) _byteswap_ulong(x)
#define le32toh(x) (x)

#define htobe64(x) _byteswap_uint64(x)
#define htole64(x) (x)
#define be64toh(x) _byteswap_uint64(x)
#define le64toh(x) (x)

#elif defined __APPLE__

/*
 * macOS has its own byte-swapping routines, so use these.
 */

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(sun) || defined(__sun) || defined(__SVR4)

/*
 * For Solaris, rely on the fallback definitions below, though use
 * Solaris-specific versions of bswap_{16,32,64}().
 */

#include <sys/byteorder.h>

#define bswap_16(x) BSWAP_16(x)
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__ANDROID__) || defined(__CYGWIN__) || defined(__GNUC__) || \
	defined(__GNU__)

#include <byteswap.h>
#include <endian.h>

#else /* if defined(__DragonFly__) || defined(__FreeBSD__) || \
       * defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) */

#endif /* Specific platform support */

/*
 * Fallback definitions.
 */

#include <inttypes.h>

#ifndef bswap_16
#define bswap_16(x)                                 \
	((uint16_t)((((uint16_t)(x)&0xff00) >> 8) | \
		    (((uint16_t)(x)&0x00ff) << 8)))
#endif /* !bswap_16 */

#ifndef bswap_32
#define bswap_32(x)                                      \
	((uint32_t)((((uint32_t)(x)&0xff000000) >> 24) | \
		    (((uint32_t)(x)&0x00ff0000) >> 8) |  \
		    (((uint32_t)(x)&0x0000ff00) << 8) |  \
		    (((uint32_t)(x)&0x000000ff) << 24)))
#endif /* !bswap_32 */

#ifndef bswap_64
#define bswap_64(x)                                                 \
	((uint64_t)((((uint64_t)(x)&0xff00000000000000ULL) >> 56) | \
		    (((uint64_t)(x)&0x00ff000000000000ULL) >> 40) | \
		    (((uint64_t)(x)&0x0000ff0000000000ULL) >> 24) | \
		    (((uint64_t)(x)&0x000000ff00000000ULL) >> 8) |  \
		    (((uint64_t)(x)&0x00000000ff000000ULL) << 8) |  \
		    (((uint64_t)(x)&0x0000000000ff0000ULL) << 24) | \
		    (((uint64_t)(x)&0x000000000000ff00ULL) << 40) | \
		    (((uint64_t)(x)&0x00000000000000ffULL) << 56)))
#endif /* !bswap_64 */

#ifndef htobe16
#if WORDS_BIGENDIAN

#define htobe16(x) (x)
#define htole16(x) bswap_16(x)
#define be16toh(x) (x)
#define le16toh(x) bswap_16(x)

#else /* WORDS_BIGENDIAN */

#define htobe16(x) bswap_16(x)
#define htole16(x) (x)
#define be16toh(x) bswap_16(x)
#define le16toh(x) (x)

#endif /* WORDS_BIGENDIAN */
#endif /* !htobe16 */

#ifndef htobe32
#if WORDS_BIGENDIAN

#define htobe32(x) (x)
#define htole32(x) bswap_32(x)
#define be32toh(x) (x)
#define le32toh(x) bswap_32(x)

#else /* WORDS_BIGENDIAN */

#define htobe32(x) bswap_32(x)
#define htole32(x) (x)
#define be32toh(x) bswap_32(x)
#define le32toh(x) (x)

#endif /* WORDS_BIGENDIAN */
#endif /* !htobe32 */

#ifndef htobe64
#if WORDS_BIGENDIAN

#define htobe64(x) (x)
#define htole64(x) bswap_64(x)
#define be64toh(x) (x)
#define le64toh(x) bswap_64(x)

#else /* WORDS_BIGENDIAN */

#define htobe64(x) bswap_64(x)
#define htole64(x) (x)
#define be64toh(x) bswap_64(x)
#define le64toh(x) (x)

#endif /* WORDS_BIGENDIAN */
#endif /* !htobe64 */
