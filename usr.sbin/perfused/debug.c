/*  $NetBSD: debug.c,v 1.1 2010/08/25 07:18:01 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "perfused.h"

#ifdef PERFUSE_DEBUG
void
perfuse_hexdump(addr, len)
	char *addr;
	size_t len;
{
	unsigned int i, j, k;

	for (i = 0; i < len; i += 16) {
		DPRINTF("%p  ", &addr[i]);
		for (j = 0; j < 16; j += 4) {
			for (k = 0; k < 4; k++) {
				if (i + j + k < len) {
					DPRINTF("%02x ", 
					       *(addr + i + j + k) & 0xff);
				} else {
					DPRINTF("   ");
				}
			}
		}

		DPRINTF("  ");
		for (j = 0; j < 16; j++) {
			char c;

			if (i + j < len) {
				c = *(addr + i + j);
				DPRINTF("%c", isprint((int)c) ? c : '.');
			} else {
				DPRINTF(" ");
			}
		}
		DPRINTF("\n");
	}

	return;
}



#endif /* PERFUSE_DEBUG */
