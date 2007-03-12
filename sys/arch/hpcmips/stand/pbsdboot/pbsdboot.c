/*	$NetBSD: pbsdboot.c,v 1.4.80.1 2007/03/12 05:48:14 rmind Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 *
 */
#include <pbsdboot.h>

#define O_RDONLY        0x0000          /* open for reading only */

int
pbsdboot(TCHAR *wkernel_name, int argc, char *argv[], struct bootinfo* bi)
{
	int i;
	void *start, end;
	void *argbuf, *p;
	struct bootinfo *bibuf;
	int fd = -1;

	stat_printf(TEXT("open %s..."), wkernel_name);
	if (CheckCancel(0) || (fd = open((char*)wkernel_name, O_RDONLY)) < 0) {
		msg_printf(MSG_ERROR, whoami, TEXT("open failed.\n"));
		stat_printf(TEXT("open %s...failed"), wkernel_name);
		goto cancel;
	}

	stat_printf(TEXT("read information from %s..."), wkernel_name);
	if (CheckCancel(0) || getinfo(fd, &start, &end) < 0) {
		stat_printf(TEXT("read information failed"), wkernel_name);
		goto cancel;
	}

	stat_printf(TEXT("create memory map..."));
	if (CheckCancel(0) || vmem_init(start, end) < 0) {
		stat_printf(TEXT("create memory map...failed"));
		goto cancel;
	}
	//vmem_dump_map();

	stat_printf(TEXT("prepare boot information..."));
	if ((argbuf = vmem_alloc()) == NULL ||
		(bibuf = (struct bootinfo*)vmem_alloc()) == NULL) {
		msg_printf(MSG_ERROR, whoami, TEXT("can't allocate argument page\n"));
		stat_printf(TEXT("prepare boot information...failed"));
		goto cancel;
	}

	memcpy(bibuf, bi, sizeof(struct bootinfo));
	for (p = &argbuf[sizeof(char*) * argc], i = 0; i < argc; i++) {
		int arglen = strlen(argv[i]) + 1;
		((char**)argbuf)[i] = p;
		memcpy(p, argv[i], arglen);
		p += arglen;
	}

	stat_printf(TEXT("loading..."));
	if (CheckCancel(0) || loadfile(fd, &start) < 0) {
		stat_printf(TEXT("loading...failed"));
		goto cancel;
	}

	/* last chance to cancel */
	if (CheckCancel(-1)) {
		goto cancel;
	}

	stat_printf(TEXT("execute kernel..."));
	vmem_exec(start, argc, (char**)argbuf, bibuf);
	stat_printf(TEXT("execute kernel...failed"));

cancel:
	if (0 <= fd) {
		close(fd);
	}
	vmem_free();

	return (-1);
}
