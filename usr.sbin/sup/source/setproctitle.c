/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef NEED_SETPROCTITLE

extern char **__environ;

void
setproctitle(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int len;
	char *pname, *p;
	char **args = __environ - 2;

	/*
	 * Keep going while it looks like a pointer. We'll stop at argc,
	 * Assume that we have < 10K args.
	 */
	while (*args > (char *)10240)
		args--;

	pname = *++args;
	*(int *)((int *)pname - 1) = 1; /* *argc = 1; */
 
	/* In case we get called again */
	if ((p = strrchr(pname, ':')) != NULL)
		*p = '\0';

	va_start(ap, fmt);
	if (fmt != NULL) {
		len = snprintf(buf, sizeof(buf), "%s: ", pname);
		if (len >= 0)
			(void)vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
	} else
		(void)snprintf(buf, sizeof(buf), "%s", pname);
	va_end(ap);
 
	(void)strcpy(pname, buf);
}
#endif

#ifdef TEST
int
main(int argc, char **argv)
{
	setproctitle("foo");
	sleep(1000);
	return 0;
}
#endif
