/*	$NetBSD: consio.c,v 1.1 2001/09/27 10:03:27 minoura Exp $	*/

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

#include <machine/stdarg.h>
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
	if (device < 0) {	/* undetemined yet */
		if (KEYCTRL & 8)
			device = ITE;
		else {
			IOCS_B_PRINT ("No keyboard; "
				      "switching to serial console...");
			device = SERIAL;
		}
	}

	switch (device) {
	case ITE:
		x68k_console_device = ITE;
		/* set palette here */
		IOCS_OS_CURON ();
		break;
	case SERIAL:
		x68k_console_device = SERIAL;
		IOCS_OS_CUROF ();
		IOCS_SET232C (SERPARAM);
	}

	return x68k_console_device;
}

int
getchar (void)
{
	int r;

	switch (x68k_console_device) {
	case ITE:
		while ((r = IOCS_B_KEYINP () & 0xff) == 0);
		return r;
	case SERIAL:
		while ((r = IOCS_INP232C () & 0xff) == 0);
		return r;
	}

	return -1;
}

void
putchar (int c)
{
	if (c == '\n')
		putchar('\r');
	switch (x68k_console_device) {
	case ITE:
		IOCS_B_PUTC (c);
	case SERIAL:
		IOCS_OUT232C (c);
	}
}

int
check_getchar (void)
{
	switch (x68k_console_device) {
	case ITE:
		return IOCS_B_KEYSNS () & 0xff;
	case SERIAL:
		return IOCS_ISNS232C () & 0xff;
	}

	return -1;
}

int
awaitkey_1sec (void)
{
	int i, c;

	while (check_getchar())
		getchar();

	for (i = 0; i < 100 && (c = check_getchar()) == 0; i++) {
		while (MFP_TIMERC > 100)
			(void) JOYA;
		while (MFP_TIMERC <= 100)
			(void) JOYA;
	}

	while (check_getchar())
		getchar();

	return c;
}

__dead void
panic(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	printf(fmt, ap);
	printf("\n");
	va_end(ap);

	exit(1);
}

void
print_title(const char *fmt, ...)
{
	va_list ap;

	/* Print the logo image here */

	va_start(ap, fmt);
	printf(fmt, ap);
	printf("\n");
	va_end(ap);
}
