/*	$NetBSD: linkaddr.c,v 1.23 2022/04/19 20:32:15 rillig Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)linkaddr.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: linkaddr.c,v 1.23 2022/04/19 20:32:15 rillig Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>

#include <assert.h>
#include <string.h>

/* States*/
#define NAMING	0
#define GOTONE	1
#define GOTTWO	2
#define RESET	3
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)
#define LETTER	(4*3)

void
link_addr(const char *addr, struct sockaddr_dl *sdl)
{
	char *cp = sdl->sdl_data;
	char *cplim = sdl->sdl_len + (char *)(void *)sdl;
	int byte = 0, state = NAMING;
	size_t newaddr = 0;	/* pacify gcc */

	_DIAGASSERT(addr != NULL);
	_DIAGASSERT(sdl != NULL);

	(void)memset(&sdl->sdl_family, 0, (size_t)sdl->sdl_len - 1);
	sdl->sdl_family = AF_LINK;
	do {
		state &= ~LETTER;
		if ((*addr >= '0') && (*addr <= '9')) {
			newaddr = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			newaddr = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			newaddr = *addr - 'A' + 10;
		} else if (*addr == 0) {
			state |= END;
		} else if (state == NAMING &&
			   (((*addr >= 'A') && (*addr <= 'Z')) ||
			   ((*addr >= 'a') && (*addr <= 'z'))))
			state |= LETTER;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case NAMING | DIGIT:
		case NAMING | LETTER:
			*cp++ = addr[-1];
			continue;
		case NAMING | DELIM:
			state = RESET;
			_DIAGASSERT(__type_fit(uint8_t, cp - sdl->sdl_data));
			sdl->sdl_nlen = (uint8_t)(cp - sdl->sdl_data);
			continue;
		case GOTTWO | DIGIT:
			*cp++ = byte;
			/* FALLTHROUGH */
		case RESET | DIGIT:
			state = GOTONE;
			byte = (int)newaddr;
			continue;
		case GOTONE | DIGIT:
			state = GOTTWO;
			byte = (int)newaddr + (byte << 4);
			continue;
		default: /* | DELIM */
			state = RESET;
			*cp++ = byte;
			byte = 0;
			continue;
		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte;
			/* FALLTHROUGH */
		case RESET | END:
			break;
		}
		break;
	} while (cp < cplim); 

	_DIAGASSERT(__type_fit(uint8_t, cp - LLADDR(sdl)));
	sdl->sdl_alen = (uint8_t)(cp - LLADDR(sdl));
	newaddr = cp - (char *)(void *)sdl;
	if (newaddr > sizeof(*sdl)) {
		_DIAGASSERT(__type_fit(uint8_t, newaddr));
		sdl->sdl_len = (uint8_t)newaddr;
	}
	return;
}

static const char hexlist[16] = "0123456789abcdef";

char *
link_ntoa(const struct sockaddr_dl *sdl)
{
	static char obuf[64];
	char *out = obuf; 
	size_t i;
	const u_char *in = (const u_char *)CLLADDR(sdl);
	const u_char *inlim = in + sdl->sdl_alen;
	int firsttime = 1;

	_DIAGASSERT(sdl != NULL);

#define ADDC(ch) \
	do { \
		if (out >= obuf + sizeof(obuf) - 1) \
			return obuf; \
		*out++ = (ch); \
	} while (0)

	/*
	 * This is not needed on the first call, as the static
	 * obuf wil be fully init'd to 0 by default.   But after
	 * obuf has been returned to userspace the first time,
	 * anything may have been written to it, so, let's be safe.
	 *
	 * (An alternative method would be to make ADDC() more
	 *  complex:
	 *	if (out < obuf + sizeof(obuf) - ((ch) != '\0'))
	 *		*out++ = (ch);
	 *  so it never returns, and the final ADDC(0) always works
	 *  but that evaluates 'ch' twice, and is slower, so ...)
	 */
	obuf[sizeof(obuf) - 1] = '\0';

	if (sdl->sdl_nlen) {
		if (sdl->sdl_nlen >= sizeof(obuf))
			i = sizeof(obuf) - 1;
		else
			i = sdl->sdl_nlen;
		(void)memcpy(obuf, sdl->sdl_data, i);
		out += i;
		if (sdl->sdl_alen)
			ADDC(':');
	}
	while (in < inlim) {
		if (firsttime)
			firsttime = 0;
		else
			ADDC('.');
		i = *in++;
		if (i > 0xf) {
			size_t j = i & 0xf;
			i >>= 4;
			ADDC(hexlist[i]);
			ADDC(hexlist[j]);
		} else
			ADDC(hexlist[i]);
	}
	ADDC('\0');
	return obuf;
}
