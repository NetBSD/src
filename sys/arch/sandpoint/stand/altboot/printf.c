/* $NetBSD: printf.c,v 1.3.2.3 2011/04/21 01:41:22 rmind Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

/*
 * printf -- format and write output using 'func' to write characters
 */

#include <sys/types.h>
#include <machine/stdarg.h>
#include <lib/libsa/stand.h>

#define MAXSTR	80

static int _doprnt(void (*)(int), const char *, va_list);
static void mkdigit(unsigned long long, int, char *);
static void sputchar(int);

static char *sbuf, *ebuf;

void
printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_doprnt(putchar, fmt, ap);
	va_end(ap);
}

void
vprintf(const char *fmt, va_list ap)
{

	_doprnt(putchar, fmt, ap);
}

int
sprintf(char *buf, const char *fmt, ...)
{
	va_list ap;

	sbuf = buf;
	ebuf = buf + -(size_t)buf - 1;
	va_start(ap, fmt);
	_doprnt(sputchar, fmt, ap);
	*sbuf = '\0';
	return (sbuf - buf);
}

int
snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;

	sbuf = buf;
	ebuf = buf + size - 1;
	va_start(ap, fmt);
	_doprnt(sputchar, fmt, ap);
	*sbuf = '\0';
	return (sbuf - buf);
}

static int
_doprnt(void (*func)(int), const char *fmt, va_list ap)
{
	int i, outcnt;
	char buf[23], *str; /* requires 23 digits in octal at most */
	int length, fmax, fmin, leading;
	int leftjust, llflag;
	char fill, sign;
	long long d;
	unsigned long long v;

	outcnt = 0;
	while ((i = *fmt++) != '\0') {
		if (i != '%') {
			(*func)(i);
			outcnt += 1;
			continue;
		}
		if (*fmt == '%') {
			(*func)(*fmt++);
			outcnt += 1;
			continue;
		}
		leftjust = (*fmt == '-');
		if (leftjust)
			fmt++;
		fill = (*fmt == '0') ? *fmt++ : ' ';
		if (*fmt == '*')
			fmin = va_arg(ap, int);
		else {
			fmin = 0;
			while ('0' <= *fmt && *fmt <= '9')
				fmin = fmin * 10 + *fmt++ - '0';
		}
		if (*fmt != '.')
			fmax = 0;
		else {
			fmt++;
			if (*fmt == '*')
				fmax = va_arg(ap, int);
			else {
				fmax = 0;
				while ('0' <= *fmt && *fmt <= '9')
					fmax = fmax * 10 + *fmt++ - '0';
			}
		}
		llflag = 0;
		if (*fmt == 'l' && *++fmt == 'l') {
			llflag = 1;
			fmt += 1;
		}
		if ((i = *fmt++) == '\0') {
			(*func)('%');
			outcnt += 1;
			break;
		}
		str = buf;
		sign = ' ';
		switch (i) {
		case 'c':
			str[0] = va_arg(ap, int);
			str[1] = '\0';
			fmax = 0;
			fill = ' ';
			break;

		case 's':
			str = va_arg(ap, char *);
			fill = ' ';
			break;

		case 'd':
			if (llflag)
				d = va_arg(ap, long long);
			else
				d = va_arg(ap, int);
			if (d < 0) {
				sign = '-' ; d = -d;
			}
			mkdigit((unsigned long long)d, 10, str);
			break;

		case 'u':
			if (llflag)
				v = va_arg(ap, unsigned long long);
			else
				v = va_arg(ap, unsigned int);
			mkdigit(v, 10, str);
			break;

		case 'o':
			if (llflag)
				v = va_arg(ap, unsigned long long);
			else
				v = va_arg(ap, unsigned int);
			mkdigit(v, 8, str);
			fmax = 0;
			break;

		case 'X':
		case 'x':
			if (llflag)
				v = va_arg(ap, unsigned long long);
			else
				v = va_arg(ap, unsigned int);
			mkdigit(v, 16, str);
			fmax = 0;
			break;

		case 'p':
			mkdigit(va_arg(ap, unsigned int), 16, str);
			fill = '0';
			fmin = 8;
			fmax = 0;
			(*func)('0'); (*func)('x');
			outcnt += 2;
			break;

		default:
			(*func)(i);
			break;
		}
		for (i = 0; str[i] != '\0'; i++)
			;
		length = i;
		if (fmin > MAXSTR || fmin < 0)
			fmin = 0;
		if (fmax > MAXSTR || fmax < 0)
			fmax = 0;
		leading = 0;
		if (fmax != 0 || fmin != 0) {
			if (fmax != 0 && length > fmax)
				length = fmax;
			if (fmin != 0)
				leading = fmin - length;
			if (sign == '-')
				--leading;
		}
		outcnt += leading + length;
		if (sign == '-')
			outcnt += 1;
		if (sign == '-' && fill == '0')
			(*func)(sign);
		if (leftjust == 0)
			for (i = 0; i < leading; i++) (*func)(fill);
		if (sign == '-' && fill == ' ')
			(*func)(sign);
		for (i = 0; i < length; i++)
			(*func)(str[i]);
		if (leftjust != 0)
			for (i = 0; i < leading; i++) (*func)(fill);
	}
	return outcnt;
}


static void
mkdigit(unsigned long long llval, int base, char *s)
{
	char ptmp[23], *t;	/* requires 22 digit in octal at most */
	int n;
	static const char hexdigit[] = "0123456789abcdef";

	n = 1;
	t = ptmp;
	*t++ = '\0';
	do {
		int d = llval % base;
		*t++ = hexdigit[d];
		llval /= base;
	} while (llval != 0 && ++n < sizeof(ptmp));
	while ((*s++ = *--t) != '\0')
		/* copy reserved digits */ ;
}

static void
sputchar(int c)
{

	if (sbuf < ebuf)
		*sbuf++ = c;
}
