/*      $Id: ssp.c,v 1.1.1.2 2009/09/04 00:27:36 gmcgarry Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>

#if defined(__lint__)
#define __constructor /* define away */
#define __destructor /* define away */
#elif defined(__PCC__)
#define __constructor _Pragma("init")
#define __destructor _Pragma("fini")
#elif defined(__GNUC__)
#define __constructor __attribute__((constructor))
#define __destructor __attribute__((destructor))
#else
#define __constructor
#define __destructor
#endif

#ifdef os_win32
#define __progname "ERROR"
#else
extern char *__progname;
#endif

int __stack_chk_guard;

void __constructor
__ssp_init(void)
{
	int fd;
	size_t sz;

	if (__stack_chk_guard != 0)
		return;

	fd = open("/dev/urandom", 0);
	if (fd > 0) {
		sz = read(fd, (char *)&__stack_chk_guard,
		    sizeof(__stack_chk_guard));
		close(fd);
		if (sz == sizeof(__stack_chk_guard))
			return;
	}

	__stack_chk_guard = 0x00000aff;
}

void
__stack_chk_fail(void)
{
	static const char msg[] = ": stack smashing attack detected\n";
	write(2, __progname, strlen(__progname));
	write(2, msg, sizeof(msg) - 1);
	abort();
}
