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

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>

#ifdef NEED_SETPROCTITLE

extern char **__environ;

void
setproctitle(const char *fmt, ...)
{
	va_list ap;
	char buf[1024];
	int len;
	char *pname, *p, *s;
	/*
	 * Assumes that stack grows down, and than environ has not bee
	 * reallocated because of setenv() required growth. Stack layout:
	 * 
	 * argc
	 * argv[0]
	 * ...
	 * argv[n]
	 * NULL
	 * environ[0]
	 * ...
	 * environ[n]
	 * NULL
	 */

	/* 1 for the first entry, 1 for the NULL */
	char **args = __environ - 2;
#ifdef _SC_ARG_MAX
	s = (char *)sysconf(_SC_ARG_MAX);
#elif defined(ARG_MAX)
	s = (char *)ARG_MAX;
#elif defined(NCARGS)
	s = (char *)NCARGS;
#else
	s = (char *)(256 * 1024);
#endif
	/*
	 * Keep going while it looks like a pointer. We'll stop at argc,
	 * Which is a lot smaller than a pointer, limited by ARG_MAX
	 */
	while (*args > s)
		args--;

	*(int *)args = 1; /* *argc = 1; */
	pname = *++args;  /* pname = argv[0] */
 
	/* In case we get called again */
	if ((p = strchr(pname, ':')) != NULL)
		*p = '\0';

	/* Just the last component of the name */
	if ((p = strrchr(pname, '/')) != NULL)
		p = p + 1;
	else
		p = pname;

	va_start(ap, fmt);
	if (fmt != NULL) {
		len = snprintf(buf, sizeof(buf), "%s: ", p);
		if (len >= 0)
			(void)vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
	} else
		(void)snprintf(buf, sizeof(buf), "%s", p);
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
