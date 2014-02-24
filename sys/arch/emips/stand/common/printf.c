/*	$NetBSD: printf.c,v 1.5 2014/02/24 07:41:15 martin Exp $	*/
/*-
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 * Copyright (c) 2006 M. Warner Losh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 * $FreeBSD: src/sys/boot/mips/emips/libemips/printf.c,v 1.2 2006/10/20 09:12:05 imp Exp $
 */

#include <lib/libsa/stand.h>
#include "common.h"

void
xputchar(int ch)
{
    if (ch == '\n')
	putchar('\r');
    putchar(ch);
}

void
printf(const char *fmt,...)
{
	va_list ap;
	const char *hex = "0123456789abcdef";
	char buf[10];
	char *s;
	unsigned u;
	int c;

	va_start(ap, fmt);
	while ((c = *fmt++)) {
		if (c == '%') {
        again:
			c = *fmt++;
			switch (c) {
			case 'l':
				goto again;
			case 'c':
				xputchar(va_arg(ap, int));
				continue;
			case 's':
				for (s = va_arg(ap, char *); s && *s; s++)
					xputchar(*s);
				continue;
			case 'd':	/* A lie, always prints unsigned */
			case 'u':
				u = va_arg(ap, unsigned);
				s = buf;
				do
					*s++ = '0' + u % 10U;
				while (u /= 10U);
			dumpbuf:;
				while (--s >= buf)
					xputchar(*s);
				continue;
			case 'x':
			case 'p':
				u = va_arg(ap, unsigned);
				s = buf;
				do
					*s++ = hex[u & 0xfu];
				while (u >>= 4);
				goto dumpbuf;
            case 0:
                return;
			}
		}
		xputchar(c);
	}
	va_end(ap);

	return;
}
