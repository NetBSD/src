/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hexdump.c,v 1.1 2017/12/08 21:51:07 christos Exp $");

#include <lib/libkern/libkern.h>
#include <sys/systm.h>

void
hexdump(const char *msg, const void *ptr, size_t len)
{
	size_t i;
	const unsigned char *u = ptr;

	if (msg)
		printf("%s: %zu bytes @ %p\n", msg, len, ptr);
        for (i = 0; i < len; ) {
                printf("%02x ", u[i++]);
                if (!(i & 0x7))
                        printf(" ");
                if (!(i & 0xf)) {
                        printf("| ");
                        for (size_t j = i - 16; j < i; j++) {
				unsigned char c = u[j];
                                printf("%c", isprint(c) ? c : '.');
			}
                        printf("\n");
                }
        }
	if ((i = (len & 0xf)) != 0) {
                for (size_t j = 16 - i; j > 0; j--) {
                        printf("   ");
                        if (!(j & 0x7) && i != 8)
                                printf(" ");
                }
                printf(" | ");
                for (size_t j = len - i; j < len; j++) {
			unsigned char c = u[j];
			printf("%c", isprint(c) ? c : '.');
		}
		printf("\n");
        }
}
