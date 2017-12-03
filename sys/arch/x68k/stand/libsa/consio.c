/*	$NetBSD: consio.c,v 1.9.12.2 2017/12/03 11:36:49 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 MINOURA Makoto.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>

#include "libx68k.h"

#include "iocs.h"
#include "consio.h"

enum {
	ITE = 0,
	SERIAL = 1,
} x68k_console_device;

int
consio_init(int device)
{

	if (device < 0) {	/* undetermined yet */
		if (KEYCTRL & 8)
			device = ITE;
		else {
			IOCS_B_PRINT("No keyboard; "
				     "switching to serial console...");
			device = SERIAL;
		}
	}

	switch (device) {
	case ITE:
		x68k_console_device = ITE;
		/* set palette here */
		IOCS_OS_CURON();
		break;
	case SERIAL:
		x68k_console_device = SERIAL;
		IOCS_OS_CUROF();
		IOCS_SET232C(SERPARAM);
	}

	return x68k_console_device;
}

int
getchar(void)
{
	int r;

	switch (x68k_console_device) {
	case ITE:
		while ((r = IOCS_B_KEYINP() & 0xff) == 0);
		return r;
	case SERIAL:
		while ((r = IOCS_INP232C() & 0xff) == 0);
		return r;
	}

	return -1;
}

void
putchar(int c)
{

	if (c == '\n')
		putchar('\r');
	switch (x68k_console_device) {
	case ITE:
		IOCS_B_PUTC(c);
		break;
	case SERIAL:
		IOCS_OUT232C(c);
		break;
	}
}

int
check_getchar(void)
{
	int keycode;

	switch (x68k_console_device) {
	case ITE:
		while ((keycode = IOCS_B_KEYSNS()) != 0) {
			keycode &= 0xff;
			if (keycode != 0) {
				/* valid ASCII code */
				return keycode;
			}
			/* discard non ASCII keys (CTRL, OPT.1 etc) */
			(void)IOCS_B_KEYINP();
		} 
		/* no input */
		return 0;
	case SERIAL:
		return IOCS_ISNS232C() & 0xff;
	}

	return -1;
}

int
awaitkey_1sec(void)
{
	int i, c;

	while (check_getchar())
		getchar();

	for (i = 0; i < 100 && (c = check_getchar()) == 0; i++) {
		while (MFP_TIMERC > 100)
			(void)JOYA;
		while (MFP_TIMERC <= 100)
			(void)JOYA;
	}

	while (check_getchar())
		getchar();

	return c;
}

extern void put_image(int, int);

void
print_title(const char *fmt, ...)
{
	va_list ap;

	if (x68k_console_device == ITE) {
		int y, y1;
		char *buf = alloca(240); /* about 3 lines */
		char *p;

		y = y1 = (IOCS_B_LOCATE(-1, -1) & 0xffff) + 1;
		put_image(8, y*16-6);
		IOCS_B_LOCATE(0, y+3);
		IOCS_B_PRINT("\360D\360a\360e\360m\360o\360n "
			     "\360l\360o\360g\360o "
			     "\360(\360C\360)\3601\3609\3609\3608\360 "
			     "\360b\360y\360 "
			     "\360M\360a\360r\360s\360h\360a\360l\360l\360 "
			     "\360K\360i\360r\360k\360 "
			     "\360M\360c\360K\360u\360s\360i\360c\360k\360.");
		va_start(ap, fmt);
		vsnprintf(buf, 240, fmt, ap);
		va_end(ap);
		while ((p = strchr(buf, '\n')) != 0) {
			*p = 0;
			IOCS_B_LOCATE(9, ++y);
			IOCS_B_PRINT(buf);
			buf = p+1;
		}
		IOCS_B_LOCATE(9, ++y);
		IOCS_B_PRINT(buf);
		IOCS_B_LOCATE(0, y1+5);
	} else {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
		printf("\n");
	}
}
