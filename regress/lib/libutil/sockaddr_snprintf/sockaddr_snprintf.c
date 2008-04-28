/*	$NetBSD: sockaddr_snprintf.c,v 1.4 2008/04/28 20:23:05 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas.
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

#include <stdio.h>
#include <string.h>
#include <util.h>

#include <sys/socket.h>	/* AF_ */

#include <netinet/in.h>		/* sin/sin6 */
#include <sys/un.h>		/* sun */
#include <netatalk/at.h>	/* sat */
#include <net/if_dl.h>		/* sdl */

static void
in(void)
{
	char buf[1024];
	int i;
	struct sockaddr_in sin4;
	const char *res;

	(void)memset(&sin4, 0, sizeof(sin4));
	sin4.sin_len = sizeof(sin4);
	sin4.sin_family = AF_INET;
	sin4.sin_port = ntohs(80);
	sin4.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
	i = sockaddr_snprintf(buf, sizeof(buf), "%f %l %p %a",
	    (struct sockaddr *)&sin4);
	if (i != 17)
		errx(1, "bad length for sin4 %d != %d", i, 17);
	if (strcmp(res = "2 16 80 127.0.0.1", buf) != 0)
		errx(1, "res='%s' != '%s'", buf, res);
}

#ifdef INET6
static void
in6(void)
{
	char buf[1024];
	int i;
	struct sockaddr_in6 sin6;
	const char *res;

	(void)memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_len = sizeof(sin6);
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = ntohs(80);
	sin6.sin6_addr = in6addr_nodelocal_allnodes;
	i = sockaddr_snprintf(buf, sizeof(buf), "%f %l %p %a",
	    (struct sockaddr *)&sin6);
	if (i != 16)
		errx(1, "bad length for sin6 %d != %d", i, 16);
	if (strcmp(res = "24 28 80 ff01::1", buf) != 0)
		errx(1, "res='%s' != '%s'", buf, res);
}
#endif /* INET6 */

static void
un(void)
{
	char buf[1024];
	int i;
	struct sockaddr_un sun;
	const char *res;

	(void)memset(&sun, 0, sizeof(sun));
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_UNIX;
	(void)strncpy(sun.sun_path, "/tmp/sock", sizeof(sun.sun_path));
	i = sockaddr_snprintf(buf, sizeof(buf), "%f %l %a",
	    (struct sockaddr *)&sun);
	if (i != 15)
		errx(1, "bad length for sun %d != %d", i, 15);
	if (strcmp(res = "1 106 /tmp/sock", buf) != 0)
		errx(1, "res='%s' != '%s'", buf, res);
}

static void
at(void)
{
	char buf[1024];
	int i;
	struct sockaddr_at sat;
	const char *res;

	(void)memset(&sat, 0, sizeof(sat));
	sat.sat_len = sizeof(sat);
	sat.sat_family = AF_APPLETALK;
	sat.sat_addr.s_net = ntohs(101);
	sat.sat_addr.s_node = 3;
	i = sockaddr_snprintf(buf, sizeof(buf), "%f %l %a",
	    (struct sockaddr *)&sat);
	if (i != 11)
		errx(1, "bad length for sat %d != %d", i, 11);
	if (strcmp(res = "16 16 101.3", buf) != 0)
		errx(1, "res='%s' != '%s'", buf, res);
}

static void
dl(void)
{
	char buf[1024];
	int i;
	struct sockaddr_dl sdl;
	const char *res;

	(void)memset(&sdl, 0, sizeof(sdl));
	sdl.sdl_len = sizeof(sdl);
	sdl.sdl_family = AF_LINK;
	sdl.sdl_index = 0;
	sdl.sdl_type = 0;
	sdl.sdl_nlen = 0;
	sdl.sdl_alen = 6;
	sdl.sdl_slen = 0;
	(void)memcpy(sdl.sdl_data, "\01\02\03\04\05\06", 6);
	i = sockaddr_snprintf(buf, sizeof(buf), "%f %l %a",
	    (struct sockaddr *)&sdl);
	if (i != 17)
		errx(1, "bad length for sdl %d != %d", i, 17);
	if (strcmp(res = "18 20 1.2.3.4.5.6", buf) != 0)
		errx(1, "res='%s' != '%s'", buf, res);
}

int
main(int argc, char *argv[])
{
	in();
#ifdef INET6
	in6();
#endif
	un();
	at();
	dl();
	return 0;
}
