/*	$NetBSD: gpt_uuid.c,v 1.7 2014/10/03 20:30:06 christos Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: gpt_uuid.c,v 1.7 2014/10/03 20:30:06 christos Exp $");
#endif

#include <stdio.h>

#include "map.h"
#include "gpt.h"

#if defined(HAVE_SYS_ENDIAN_H) || ! defined(HAVE_NBTOOL_CONFIG_H)
#include <sys/endian.h>
#endif


const gpt_uuid_t gpt_uuid_nil;

struct dce_uuid {
	uint32_t	time_low;
	uint16_t	time_mid;
	uint16_t	time_hi_and_version;
	uint8_t		clock_seq_hi_and_reserved;
	uint8_t		clock_seq_low;
	uint8_t		node[6];
};

struct kern_uuid {
	uint64_t	ll;
	uint16_t	seq;
	uint8_t		node[6];
};

static const struct {
	struct dce_uuid u;
	const char *n;
	const char *d;
} gpt_nv[] = {
	{ GPT_ENT_TYPE_APPLE_HFS, "apple", "Apple HFS" },
	{ GPT_ENT_TYPE_BIOS, "bios", "BIOS Boot" },
	{ GPT_ENT_TYPE_EFI, "efi", "EFI System" },
	{ GPT_ENT_TYPE_FREEBSD, "fbsd-legacy", "FreeBSD legacy" },
	{ GPT_ENT_TYPE_FREEBSD_SWAP, "fbsd-swap", "FreeBSD swap" },
	{ GPT_ENT_TYPE_FREEBSD_UFS, "fbsd-ufs", "FreeBSD UFS/UFS2" },
	{ GPT_ENT_TYPE_FREEBSD_VINUM, "fbsd-vinum", "FreeBSD vinum" },
	{ GPT_ENT_TYPE_FREEBSD_ZFS, "fbsd-zfs", "FreeBSD ZFS" },
	{ GPT_ENT_TYPE_LINUX_DATA, "linux-data", "Linux data" },
	{ GPT_ENT_TYPE_LINUX_SWAP, "linux-swap", "Linux swap" },
	{ GPT_ENT_TYPE_MS_BASIC_DATA, "windows", "Windows basic data" },
	{ GPT_ENT_TYPE_MS_RESERVED, "windows-reserved", "Windows reserved" },
	{ GPT_ENT_TYPE_NETBSD_CCD, "ccd", "NetBSD ccd component" },
	{ GPT_ENT_TYPE_NETBSD_CGD, "cgd", "NetBSD Cryptographic Disk" },
	{ GPT_ENT_TYPE_NETBSD_FFS, "ffs", "NetBSD FFSv1/FFSv2" },
	{ GPT_ENT_TYPE_NETBSD_LFS, "lfs", "NetBSD LFS" },
	{ GPT_ENT_TYPE_NETBSD_RAIDFRAME, "raid",
	    "NetBSD RAIDFrame component" },
	{ GPT_ENT_TYPE_NETBSD_SWAP, "swap", "NetBSD swap" },
};

static void
gpt_uuid_to_dce(const gpt_uuid_t buf, struct dce_uuid *uuid)
{
	const uint8_t *p = buf;
	size_t i;

	uuid->time_low = le32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = le16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < sizeof(uuid->node); i++)
		uuid->node[i] = p[10 + i];
}

static void
gpt_dce_to_uuid(const struct dce_uuid *uuid, uint8_t *buf)
{
	uint8_t *p = buf;
	size_t i;

	le32enc(p, uuid->time_low);
	le16enc(p + 4, uuid->time_mid);
	le16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < sizeof(uuid->node); i++)
		p[10 + i] = uuid->node[i];
}

static int
gpt_uuid_numeric(char *buf, size_t bufsiz, const struct dce_uuid *u)
{
	return snprintf(buf, bufsiz,
	    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	    u->time_low, u->time_mid, u->time_hi_and_version,
	    u->clock_seq_hi_and_reserved, u->clock_seq_low, u->node[0],
	    u->node[1], u->node[2], u->node[3], u->node[4], u->node[5]);
}


static int
gpt_uuid_symbolic(char *buf, size_t bufsiz, const struct dce_uuid *u)
{
	size_t i;

	for (i = 0; i < __arraycount(gpt_nv); i++)
		if (memcmp(&gpt_nv[i].u, u, sizeof(*u)) == 0)
			return strlcpy(buf, gpt_nv[i].n, bufsiz);
	return -1;
}

static int
gpt_uuid_descriptive(char *buf, size_t bufsiz, const struct dce_uuid *u)
{
	size_t i;

	for (i = 0; i < __arraycount(gpt_nv); i++)
		if (memcmp(&gpt_nv[i].u, u, sizeof(*u)) == 0)
			return strlcpy(buf, gpt_nv[i].d, bufsiz);
	return -1;
}

int
gpt_uuid_snprintf(char *buf, size_t bufsiz, const char *fmt,
    const gpt_uuid_t uu)
{
	struct dce_uuid u;
	gpt_uuid_to_dce(uu, &u);

	if (fmt[1] == 's') {
		int r;
		if ((r = gpt_uuid_symbolic(buf, bufsiz, &u)) != -1)
			return r;
	}
	if (fmt[1] == 'l') {
		int r;
		if ((r = gpt_uuid_descriptive(buf, bufsiz, &u)) != -1)
			return r;
	}
	return gpt_uuid_numeric(buf, bufsiz, &u);
}

static int
gpt_uuid_parse_numeric(const char *s, struct dce_uuid *u)
{
	int n;

	if (s == NULL || *s == '\0') {
		memset(u, 0, sizeof(*u));
		return 0;
	}

	n = sscanf(s,
	    "%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
	    &u->time_low, &u->time_mid, &u->time_hi_and_version,
	    &u->clock_seq_hi_and_reserved, &u->clock_seq_low, &u->node[0],
	    &u->node[1], &u->node[2], &u->node[3], &u->node[4], &u->node[5]);

	/* Make sure we have all conversions. */
	if (n != 11)
		return -1;

	/* We have a successful scan. Check semantics... */
	n = u->clock_seq_hi_and_reserved;
	if ((n & 0x80) != 0x00 &&			/* variant 0? */
	    (n & 0xc0) != 0x80 &&			/* variant 1? */
	    (n & 0xe0) != 0xc0) 			/* variant 2? */
		return -1;
	return 0;
}

static int
gpt_uuid_parse_symbolic(const char *s, struct dce_uuid *u)
{
	size_t i;

	for (i = 0; i < __arraycount(gpt_nv); i++)
		if (strcmp(gpt_nv[i].n, s) == 0) {
			*u = gpt_nv[i].u;
			return 0;
		}
	return -1;
}

int
gpt_uuid_parse(const char *s, gpt_uuid_t uuid)
{
	struct dce_uuid u;

	if (gpt_uuid_parse_numeric(s, &u) != -1) {
		gpt_dce_to_uuid(&u, uuid);
		return 0;
	}

	if (gpt_uuid_parse_symbolic(s, &u) == -1)
		return -1;

	gpt_dce_to_uuid(&u, uuid);
	return 0;
}

void
gpt_uuid_create(gpt_type_t t, gpt_uuid_t u, uint16_t *b, size_t s)
{
	gpt_dce_to_uuid(&gpt_nv[t].u, u);
	if (b)
		utf8_to_utf16((const uint8_t *)gpt_nv[t].d, b, s / sizeof(*b));
}

#if !defined(HAVE_NBTOOL_CONFIG_H)
#include <sys/types.h>
#include <sys/uuid.h>
#else
#include <time.h>
/*
 * Get the current time as a 5x bit count of 100000-microsecond intervals
 * since 00:00:00.00, October 15,1582. We apply a magic offset to convert
 * the Unix time since 00:00:00.00, January 1, 1970 to the date of the
 * Gregorian reform to the Christian calendar.
 */
static uint64_t
uuid_time(void)
{
	struct timeval tv;
	uint64_t xtime = 0x01B21DD213814000LL;

	(void)gettimeofday(&tv, NULL);
	xtime += (uint64_t)tv.tv_sec * 10000000LL;
	xtime += (uint64_t)(tv.tv_usec / 100000);
	return (xtime & ((1LL << 60) - 1LL));
}

/*
 * No portable way to get ethernet, use hostid instead
 */
static void
uuid_node(uint8_t node[6])
{
	long hid = gethostid();
	node[0] = 'N';
	node[1] = 'B';
	node[2] = (hid >> 24) & 0xff;
	node[3] = (hid >> 16) & 0xff;
	node[4] = (hid >>  8) & 0xff;
	node[5] = (hid >>  0) & 0xff;
}

static void
uuid_generate(void *u, uint64_t *timep, int count)
{
	static struct kern_uuid uuid_last;
	uint64_t xtime, ltime;
	uint16_t lseq;
	struct kern_uuid *uuid = u;

	uuid_node(uuid->node);
	xtime = uuid_time();
	*timep = xtime;

	if (uuid_last.ll == 0LL || uuid_last.node[0] != uuid->node[0] ||
	    uuid_last.node[1] != uuid->node[1] ||
	    uuid_last.node[2] != uuid->node[2]) {
		srandom((unsigned int) xtime);
		uuid->seq = (uint16_t)random() & 0x3fff;
	} else if (uuid_last.ll >= xtime)
		uuid->seq = (uuid_last.seq + 1) & 0x3fff;

	uuid_last = *uuid;
	uuid_last.ll = (xtime + count - 1) & ((1LL << 60) - 1LL);
}

static void
uuidgen(struct dce_uuid *store, int count)
{
	uint64_t xtime;
	struct kern_uuid uuid;
	int i;

	/* Generate the base UUID. */
	uuid_generate(&uuid, &xtime, count);

	for (i = 0; i < count; xtime++, i++) {
		/* Set time and version (=1) and deal with byte order. */
		store[i].time_low = (uint32_t)xtime;
		store[i].time_mid = (uint16_t)(xtime >> 32);
		store[i].time_hi_and_version =
		    ((uint16_t)(xtime >> 48) & 0xfff) | (1 << 12);
		store[i].clock_seq_hi_and_reserved = (uuid.seq >> 16) | 0x80;
		store[i].clock_seq_low = uuid.seq & 0xff;
		memcpy(store[i].node, uuid.node, sizeof(uuid.node));
	}
}
#endif

void
gpt_uuid_generate(gpt_uuid_t t)
{
	struct dce_uuid u;

	uuidgen((void *)&u, 1);
	gpt_dce_to_uuid(&u, t);
}
