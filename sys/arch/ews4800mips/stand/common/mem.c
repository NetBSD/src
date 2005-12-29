/*	$NetBSD: mem.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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


#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/param.h>
#include <machine/bfs.h>

#include "local.h"
#include "cmd.h"

/*
 * Dump 350 GA-ROM
 * >> mem g 0xf7e00000 4 4 0x8000 garom.bin
 */

void mem_write(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void mem_read(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
boolean_t __ga_rom;

int
cmd_mem(int argc, char *argp[], int interactive)
{
	const char *filename = 0;
	struct bfs *bfs;
	uint32_t a[4], v;
	int i, c;
	size_t size;
	void *p;

	if (argc < 6)
		goto error;

	for (i = 0; i < 4; i++)
		a[i] = strtoul(argp[2 + i], 0, 0);
	c = *argp[1];

	if (c == 'g') {
		size = a[3];	/* GA-ROM special */
		__ga_rom = TRUE;
	} else {
		size = a[1] * a[3];
		__ga_rom = FALSE;
	}

	p = 0;
	if ((c == 'd') || (c == 'b') || (c == 'g')) {
		if (argc < 7)
			goto error;
		filename = argp[6];
		if (strlen(filename) > BFS_FILENAME_MAXLEN) {
			printf("too long filename. (max %d)\n",
			    BFS_FILENAME_MAXLEN);
			return 1;
		}
		if ((p = alloc(size)) == 0) {
			printf("can't allocate buffer.\n");
			return 1;
		}

		if (bfs_init(&bfs) != 0) {
			printf("no BFS partition.\n");
			free(p, size);
			return 1;
		}
	}

	switch (c) {
	default:
		goto error;
	case 'r':
		mem_read(a[0], a[1], a[2], a[3], 0);
		break;

	case 'w':
		if (argc < 7)
			goto error;
		v = strtoul(argp[6], 0, 0);
		mem_write(a[0], a[1], a[2], a[3], (uint32_t)&v);
		break;

	case 'd':
		mem_read(a[0], a[1], a[2], a[3], (uint32_t)p);
		if (bfs_file_write(bfs, filename, p, size) != 0)
			printf("BFS write failed.\n");
		bfs_fini(bfs);
		break;

	case 'b':
		if (bfs_file_read(bfs, filename, p, size, 0) != 0)
			printf("BFS read failed.\n");
		else
			mem_write(a[0], a[1], a[2], a[3], (uint32_t)p);
		bfs_fini(bfs);
		break;

	case 'g':	/* GA-ROM special */
		mem_read(a[0], a[1], a[2], a[3], (uint32_t)p);
		if (bfs_file_write(bfs, filename, p, size) != 0)
			printf("BFS write failed.\n");
		bfs_fini(bfs);
		break;

	}

	return 0;
 error:
	printf("mem r addr access_byte stride count\n");
	printf("mem w addr access_byte stride count value\n");
	printf("mem d addr access_byte stride count filename\n");
	printf("mem b addr access_byte stride count filename\n");
	printf("mem g addr access_byte stride count filename (GA-ROM only)\n");
	return 1;
}

void
mem_write(uint32_t dst_addr, uint32_t access, uint32_t stride,
    uint32_t count, uint32_t src_addr)
{
	int i;

	printf("write: addr=%p access=%dbyte stride=%dbyte count=%d Y/N?",
	    (void *)dst_addr, access, stride, count);

	if (!prompt_yesno(1))
		return;

	switch (access) {
	default:
		printf("invalid %dbyte access.\n", access);
		break;
	case 1:
		if (count == 1)
			printf("%p = 0x%x\n",
			    (uint8_t *)dst_addr,  *(uint8_t *)src_addr);
		for (i = 0; i < count; i++,
		    dst_addr += stride, src_addr += stride) {
			*(uint8_t *)dst_addr = *(uint8_t *)src_addr;
		}
	case 2:
		if (count == 1)
			printf("%p = 0x%x\n",
			    (uint16_t *)dst_addr, *(uint16_t *)src_addr);
		for (i = 0; i < count; i++,
		    dst_addr += stride, src_addr += stride) {
			*(uint16_t *)dst_addr = *(uint16_t *)src_addr;
		}
	case 4:
		if (count == 1)
			printf("%p = 0x%x\n",
			    (uint32_t *)dst_addr, *(uint32_t *)src_addr);
		for (i = 0; i < count; i++,
		    dst_addr += stride, src_addr += stride) {
			*(uint32_t *)dst_addr = *(uint32_t *)src_addr;
		}
	}
}

void
mem_read(uint32_t src_addr, uint32_t access, uint32_t stride,
    uint32_t count, uint32_t dst_addr)
{
	uint32_t v = 0;
	int i;

	printf("read: addr=%p access=%dbyte stride=%dbyte count=%d. Y/N?\n",
	    (void *)src_addr, access, stride, count);

	if (!prompt_yesno(1))
		return;

	if (dst_addr == 0) {
		for (i = 0; i < count; i++) {
			switch (access) {
			default:
				printf("invalid %dbyte access.\n", access);
				break;
			case 1:
				v = *(uint8_t *)src_addr;
				break;
			case 2:
				v = *(uint16_t *)src_addr;
				break;
			case 4:
				v = *(uint32_t *)src_addr;
				break;
			}
			printf("%p: > 0x%x\n", (void *)src_addr, v);
			src_addr += stride;
		}
	} else {
		switch (access) {
		default:
			printf("invalid %dbyte access.\n", access);
			break;
		case 1:
			for (i = 0; i < count; i++,
			    src_addr += stride, dst_addr += stride)
				*(uint8_t *)dst_addr = *(uint8_t *)src_addr;
			break;
		case 2:
			for (i = 0; i < count; i++,
			    src_addr += stride, dst_addr += stride)
				*(uint16_t *)dst_addr = *(uint16_t *)src_addr;
			break;
		case 4:
			if (__ga_rom) {
				for (i = 0; i < count; i++,
				    src_addr += 4, dst_addr += 1)
					*(uint8_t *)dst_addr =
					    *(uint32_t *)src_addr;
			} else {
				for (i = 0; i < count; i++,
				    src_addr += stride, dst_addr += stride)
					*(uint32_t *)dst_addr =
					    *(uint32_t *)src_addr;
			}
			break;
		}
	}
	printf("done.\n");
}
