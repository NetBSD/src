/*	$NetBSD: parse_args.c,v 1.10 2005/06/28 21:03:02 junyoung Exp $	*/

/*-
 * Copyright (c) 1995 Theo de Raadt
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <machine/prom.h>
#include <sys/boot_flag.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

#define KERNEL_NAME "netbsd"

void
parse_args(char **filep, int *flagp, int *partp)
{
	char *name = KERNEL_NAME, *ptr;
	int howto = 0, part = 0;
	char c;

	if (bugargs.arg_start != bugargs.arg_end) {
		ptr = bugargs.arg_start;
		while ((c = *ptr)) {
			while (c == ' ')
				c = *++ptr;
			if (c == '\0')
				return;
			if (c != '-') {
				if (ptr[1] == ':') {
					part = (int) (*ptr - 'A');
					if (part >= MAXPARTITIONS)
						part -= 0x20;
					if (part < 0 || part >= MAXPARTITIONS)
						part = 0;
					if (ptr[2] == ' ' || ptr[2] == '\0') {
						ptr += 2;
						continue;
					}
					name = &(ptr[2]);
				} else
					name = ptr;
				while ((c = *++ptr) && c != ' ')
					;
				if (c)
					*ptr++ = 0;
				continue;
			}
			while ((c = *++ptr) && c != ' ')
				BOOT_FLAG(c, howto);
		}
	}
	*flagp = howto;
	*filep = name;
	*partp = part;
}
