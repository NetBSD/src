/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2008 Roy Marples <roy@marples.name>
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

#define THIRTY_YEARS_IN_SECONDS    946707779

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "duid.h"
#include "net.h"

size_t
get_duid(unsigned char *duid, const struct interface *iface)
{
	FILE *f;
	uint16_t type = 0;
	uint16_t hw = 0;
	uint32_t ul;
	time_t t;
	int x = 0;
	unsigned char *p = duid;
	size_t len = 0;
	char *line;

	/* If we already have a DUID then use it as it's never supposed
	 * to change once we have one even if the interfaces do */
	if ((f = fopen(DUID, "r"))) {
		while ((line = get_line(f))) {
			len = hwaddr_aton(NULL, line);
			if (len && len <= DUID_LEN) {
				hwaddr_aton(duid, line);
				break;
			}
			len = 0;
		}
		fclose(f);
		if (len)
			return len;
	} else {
		if (errno != ENOENT)
			return 0;
	}

	/* No file? OK, lets make one based on our interface */
	if (!(f = fopen(DUID, "w")))
		return 0;
	type = htons(1); /* DUI-D-LLT */
	memcpy(p, &type, 2);
	p += 2;
	hw = htons(iface->family);
	memcpy(p, &hw, 2);
	p += 2;
	/* time returns seconds from jan 1 1970, but DUID-LLT is
	 * seconds from jan 1 2000 modulo 2^32 */
	t = time(NULL) - THIRTY_YEARS_IN_SECONDS;
	ul = htonl(t & 0xffffffff);
	memcpy(p, &ul, 4);
	p += 4;
	/* Finally, add the MAC address of the interface */
	memcpy(p, iface->hwaddr, iface->hwlen);
	p += iface->hwlen;
	len = p - duid;
	x = fprintf(f, "%s\n", hwaddr_ntoa(duid, len));
	fclose(f);
	/* Failed to write the duid? scrub it, we cannot use it */
	if (x < 1) {
		len = 0;
		unlink(DUID);
	}
	return len;
}
