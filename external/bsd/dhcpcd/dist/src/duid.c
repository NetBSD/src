/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2019 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define	UUID_LEN	36
#define	DUID_TIME_EPOCH 946684800
#define	DUID_LLT	1
#define	DUID_LL		3
#define	DUID_UUID	4

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef BSD
#  include <sys/sysctl.h>
#endif

#include <arpa/inet.h>

#include <net/if.h>
#include <net/if_arp.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "dhcpcd.h"
#include "duid.h"
#include "logerr.h"

static size_t
duid_machineuuid(char *uuid, size_t uuid_len)
{
	int r;
	size_t len = uuid_len;

#if defined(HW_UUID) /* OpenBSD */
	int mib[] = { CTL_HW, HW_UUID };

	r = sysctl(mib, sizeof(mib)/sizeof(mib[0]), uuid, &len, NULL, 0);
#elif defined(KERN_HOSTUUID) /* FreeBSD */
	int mib[] = { CTL_KERN, KERN_HOSTUUID };

	r = sysctl(mib, sizeof(mib)/sizeof(mib[0]), uuid, &len, NULL, 0);
#elif defined(__NetBSD__)
	r = sysctlbyname("machdep.dmi.system-uuid", uuid, &len, NULL, 0);
#elif defined(__linux__)
	FILE *fp;

	fp = fopen("/sys/class/dmi/id/product_uuid", "r");
	if (fp == NULL)
		return 0;
	if (fgets(uuid, (int)uuid_len, fp) == NULL) {
		fclose(fp);
		return 0;
	}
	len = strlen(uuid) + 1;
	fclose(fp);
	r = len == 1 ? -1 : 0;
#else
	UNUSED(uuid);
	r = -1;
	errno = ENOSYS;
#endif

	if (r == -1)
		return 0;
	return len;
}

static size_t
duid_make_uuid(uint8_t *d)
{
	uint16_t type = htons(DUID_UUID);
	char uuid[UUID_LEN + 1];
	size_t l;

	if (duid_machineuuid(uuid, sizeof(uuid)) != sizeof(uuid))
		return 0;

	/* All zeros UUID is not valid */
	if (strcmp("00000000-0000-0000-0000-000000000000", uuid) == 0)
		return 0;

	memcpy(d, &type, sizeof(type));
	l = sizeof(type);
	d += sizeof(type);
	l += hwaddr_aton(d, uuid);
	return l;
}

static size_t
duid_make(uint8_t *d, const struct interface *ifp, uint16_t type)
{
	uint8_t *p;
	uint16_t u16;
	time_t t;
	uint32_t u32;

	p = d;
	u16 = htons(type);
	memcpy(p, &u16, 2);
	p += 2;
	u16 = htons(ifp->family);
	memcpy(p, &u16, 2);
	p += 2;
	if (type == DUID_LLT) {
		/* time returns seconds from jan 1 1970, but DUID-LLT is
		 * seconds from jan 1 2000 modulo 2^32 */
		t = time(NULL) - DUID_TIME_EPOCH;
		u32 = htonl((uint32_t)t & 0xffffffff);
		memcpy(p, &u32, 4);
		p += 4;
	}
	/* Finally, add the MAC address of the interface */
	memcpy(p, ifp->hwaddr, ifp->hwlen);
	p += ifp->hwlen;
	return (size_t)(p - d);
}

#define DUID_STRLEN DUID_LEN * 3
static size_t
duid_get(uint8_t **d, const struct interface *ifp)
{
	FILE *fp;
	uint8_t *data;
	size_t len;
	int x = 0;
	char line[DUID_STRLEN];
	const struct interface *ifp2;

	/* If we already have a DUID then use it as it's never supposed
	 * to change once we have one even if the interfaces do */
	if ((len = read_hwaddr_aton(&data, DUID)) != 0) {
		if (len <= DUID_LEN) {
			*d = data;
			return len;
		}
		logerrx("DUID too big (max %u): %s", DUID_LEN, DUID);
		/* Keep the buffer, will assign below. */
	} else {
		if (errno != ENOENT)
			logerr("%s", DUID);
		if ((data = malloc(DUID_LEN)) == NULL) {
			logerr(__func__);
			return 0;
		}
	}

	/* Regardless of what happens we will create a DUID to use. */
	*d = data;

	/* No file? OK, lets make one based the machines UUID */
	len = duid_make_uuid(data);
	if (len > 0)
		return len;

	/* No UUID? OK, lets make one based on our interface */
	if (ifp->hwlen == 0) {
		logwarnx("%s: does not have hardware address", ifp->name);
		TAILQ_FOREACH(ifp2, ifp->ctx->ifaces, next) {
			if (ifp2->hwlen != 0)
				break;
		}
		if (ifp2) {
			ifp = ifp2;
			logwarnx("picked interface %s to generate a DUID",
			    ifp->name);
		} else {
			logwarnx("no interfaces have a fixed hardware "
			    "address");
			return duid_make(data, ifp, DUID_LL);
		}
	}

	if (!(fp = fopen(DUID, "w"))) {
		logerr("%s", DUID);
		return duid_make(data, ifp, DUID_LL);
	}
	len = duid_make(data, ifp, DUID_LLT);
	x = fprintf(fp, "%s\n", hwaddr_ntoa(data, len, line, sizeof(line)));
	if (fclose(fp) == EOF)
		x = -1;
	/* Failed to write the duid? scrub it, we cannot use it */
	if (x < 1) {
		logerr("%s", DUID);
		unlink(DUID);
		return duid_make(data, ifp, DUID_LL);
	}
	return len;
}

size_t duid_init(const struct interface *ifp)
{

	if (ifp->ctx->duid == NULL)
		ifp->ctx->duid_len = duid_get(&ifp->ctx->duid, ifp);
	return ifp->ctx->duid_len;
}
