/*	$NetBSD: dl_print.c,v 1.8 2021/05/27 13:40:38 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: dl_print.c,v 1.8 2021/05/27 13:40:38 christos Exp $");
#include <sys/systm.h>
#else
__RCSID("$NetBSD: dl_print.c,v 1.8 2021/05/27 13:40:38 christos Exp $");
#include <stdio.h>
static const char hexdigits[] = "0123456789abcdef";
#endif
#include <net/if_dl.h>

static int
lla_snprintf1(char *dst, size_t dst_len, const void *src, size_t src_len)
{
	char *dp;
	const uint8_t *sp, *ep;

	dp = dst;
	sp = (const uint8_t *)src;
	ep = sp + src_len;
	while (sp < ep) {
		if (dst_len-- == 0)
			break;
		*dp++ = hexdigits[(*sp) >> 4];
		if (dst_len-- == 0)
			break;
		*dp++ = hexdigits[(*sp++) & 0xf];
		if (dst_len-- == 0)
			break;
		*dp++ = ':';
	}
	if (dp != dst)
		*--dp = '\0';

	return src_len ? (int)(src_len * 3) - 1 : 0;
}

char *
lla_snprintf(char *dst, size_t dst_len, const void *src, size_t src_len)
{
	(void)lla_snprintf1(dst, dst_len, src, src_len);
	return dst;
}

#define clip(a, b) ((a) > (size_t)(b) ? (a) - (size_t)(b) : 0)

int
dl_print(char *buf, size_t len, const struct dl_addr *dl)
{
	int l = snprintf(buf, len, "%.*s/%hhu#",
	    (int)dl->dl_nlen, dl->dl_data, dl->dl_type);
	if (l == -1)
		return l;
	int ll = lla_snprintf1(buf + l, clip(len, l),
	    dl->dl_data + dl->dl_nlen, dl->dl_alen);
	if (ll == -1)
		return ll;

	return ll + l;
}

int
sdl_print(char *buf, size_t len, const void *v)
{
	const struct sockaddr_dl *sdl = v;

	if (sdl->sdl_slen == 0 && sdl->sdl_nlen == 0 && sdl->sdl_alen == 0)
		return snprintf(buf, len, "link#%hu", sdl->sdl_index);

	if (len > 0) {
		buf[0] = '[';
		len--;
	}
	int l = dl_print(buf + 1, len, &sdl->sdl_addr);
	if (l == -1)
		return l;
	l++;
	len++;
	int ll = snprintf(buf + l, clip(len, l), "]:%hu", sdl->sdl_index);
	if (ll == -1)
		return ll;
	return ll + l;
}
