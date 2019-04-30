/*	$NetBSD: dl_print.c,v 1.6 2019/04/30 20:56:32 kre Exp $	*/

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
#include <sys/cdefs.h>
#include <sys/types.h>

#ifdef _KERNEL
__KERNEL_RCSID(0, "$NetBSD: dl_print.c,v 1.6 2019/04/30 20:56:32 kre Exp $");
#include <sys/systm.h>
#else
__RCSID("$NetBSD: dl_print.c,v 1.6 2019/04/30 20:56:32 kre Exp $");
#include <stdio.h>
static const char hexdigits[] = "0123456789abcdef";
#endif
#include <net/if_dl.h>

char *
lla_snprintf(char *dst, size_t dst_len, const void *src, size_t src_len)
{
	char *dp;
	const uint8_t *sp, *ep;

	if (src_len == 0 || dst_len < 3) {
		if (dst_len != 0)
			dst[0] = '\0';
		return NULL;
	}

	dp = dst;
	sp = (const uint8_t *)src;
	ep = sp + src_len;
	while (sp < ep) {
		if (dst_len < 3)
			break;
		dst_len -= 3;
		*dp++ = hexdigits[(*sp) >> 4];
		*dp++ = hexdigits[(*sp++) & 0xf];
		*dp++ = ':';
	}
	*--dp = 0;

	return dst;
}

int
dl_print(char *buf, size_t len, const struct dl_addr *dl)
{
	char abuf[256 * 3];

	lla_snprintf(abuf, sizeof(abuf), dl->dl_data+dl->dl_nlen, dl->dl_alen);
	return snprintf(buf, len, "%.*s/%hhu#%s",
	    (int)dl->dl_nlen, dl->dl_data, dl->dl_type, abuf);
}

int
sdl_print(char *buf, size_t len, const void *v)
{
	const struct sockaddr_dl *sdl = v;
	char abuf[LINK_ADDRSTRLEN];

	if (sdl->sdl_slen == 0 && sdl->sdl_nlen == 0 && sdl->sdl_alen == 0)
		return snprintf(buf, len, "link#%hu", sdl->sdl_index);

	dl_print(abuf, sizeof(abuf), &sdl->sdl_addr);
	return snprintf(buf, len, "[%s]:%hu", abuf, sdl->sdl_index);
}
