/* $NetBSD: i28f128.c,v 1.3 2003/08/09 08:01:47 igy Exp $ */

/*
 * Copyright (c) 2003 Naoto Shimazaki.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY NAOTO SHIMAZAKI AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE NAOTO OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Flash Memory Writer
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i28f128.c,v 1.3 2003/08/09 08:01:47 igy Exp $");

#include <lib/libsa/stand.h>

#include "extern.h"

#include "i28f128reg.h"

#define USE_TWIDDLE

/*
 * XXX
 * this function is too much specific for the device.
 */
int
i28f128_probe(void *base)
{
	static const u_int8_t	vendor_code[] = {
		0x89,	/* manufacturer code: 	intel */
		0x18,	/* device code:		28F128 */
	};

	static const u_int8_t	idstr[] = {
		'Q', 'R', 'Y',
		0x01, 0x00,
		0x31, 0x00,
		0xff
	};

	int	i;

	/* start Common Flash Interface Query */
	REGWRITE_2(base, 0, 0x98);

	/* read CFI Query ID string */
	for (i = 0; idstr[i] != 0xff; i++) {
		if (REGREAD_2(base, (0x10 + i) << 1) != idstr[i])
			return 1;
	}

	/* read manufacturer code and device code */
	if (REGREAD_2(base, 0x00) != vendor_code[0])
		return 1;
	if (REGREAD_2(base, 0x02) != vendor_code[1])
		return 1;
	
	REGWRITE_2(base, 0, 0xff);
	return 0;
}

static int
block_erase(void *addr)
{
	int	status;

	REGWRITE_2(addr, 0, I28F128_BLK_ERASE_1ST);
	REGWRITE_2(addr, 0, I28F128_BLK_ERASE_2ND);

	do {
		status = REGREAD_2(addr, 0);
	} while (!ISSET(status, I28F128_S_READY));

	REGWRITE_2(addr, 0, I28F128_CLEAR_STATUS);
	REGWRITE_2(addr, 0, I28F128_RESET);

	return status & (I28F128_S_ERASE_SUSPEND
			 | I28F128_S_ERASE_ERROR
			 | I28F128_S_BLOCK_LOCKED);
}

static int
word_program(void *addr, u_int16_t data)
{
	int	status;

	REGWRITE_2(addr, 0, I28F128_WORDBYTE_PROG);
	REGWRITE_2(addr, 0, data);
	
	do {
		status = REGREAD_2(addr, 0);
	} while (!ISSET(status, I28F128_S_READY));

	REGWRITE_2(addr, 0, I28F128_CLEAR_STATUS);
	REGWRITE_2(addr, 0, I28F128_RESET);

	return status & (I28F128_S_PROG_ERROR
			 | I28F128_S_LOW_VOLTAGE
			 | I28F128_S_PROG_SUSPEND
			 | I28F128_S_BLOCK_LOCKED);
}

static int
block_write(void *dst, const void *src)
{
	int		status;
	const u_int16_t	*p;
	u_int16_t	*q;
	const u_int16_t	*fence;
	int		i;
	const int	wbuf_count = I28F128_WBUF_SIZE >> 1;

	/* dst must be aligned to block boundary. */
	if (I28F128_BLOCK_MASK & (u_int32_t) dst)
		return -1;

	if (memcmp(dst, src, I28F128_BLOCK_SIZE) == 0)
		return 0;

	if ((status = block_erase(dst)) != 0)
		return status;

	p = src;
	q = dst;
	fence = p + (I28F128_BLOCK_SIZE >> 1);
	do {
		do {
			REGWRITE_2(dst, 0, I28F128_WRITE_BUFFER);
			status = REGREAD_2(dst, 0);
		} while (!ISSET(status, I28F128_XS_BUF_AVAIL));

		REGWRITE_2(dst, 0, wbuf_count - 1);

		for (i = wbuf_count; i > 0; i--, p++, q++)
			REGWRITE_2(q, 0, *p);

		REGWRITE_2(dst, 0, I28F128_WBUF_CONFIRM);

		do {
			REGWRITE_2(dst, 0, I28F128_READ_STATUS);
			status = REGREAD_2(dst, 0);
		} while (!(status & I28F128_S_READY));

	} while (p < fence);

	REGWRITE_2(dst, 0, I28F128_CLEAR_STATUS);
	REGWRITE_2(dst, 0, I28F128_RESET);

	return 0;
}

int
i28f128_region_write(void *dst, const void *src, size_t len)
{
	int		status;
	const u_int16_t	*p = src;
	u_int16_t	*q = dst;

	/* dst must be aligned to block boundary. */
	if (I28F128_BLOCK_MASK & (u_int32_t) dst)
		return -1;

	while (len >= I28F128_BLOCK_SIZE) {
		if ((status = block_write(q, p)) != 0)
			return status;
		putchar('b');
		p += I28F128_BLOCK_SIZE >> 1;
		q += I28F128_BLOCK_SIZE >> 1;
		len -= I28F128_BLOCK_SIZE;
	}

	if (len > 0) {
		if (memcmp(p, q, len) == 0)
			return 0;
		if ((status = block_erase(q)) != 0)
			return status;
		for (; len > 0; len -= 2) {
#ifdef USE_TWIDDLE
			if (((u_int32_t) q % 4096) == 0)
				twiddle();
#endif
			if ((status = word_program(q++, *p++)) != 0)
				return status;
		}
		printf("w");
	}

	putchar('\n');
	return 0;
}
