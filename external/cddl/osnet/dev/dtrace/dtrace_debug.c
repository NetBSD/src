/*	$NetBSD: dtrace_debug.c,v 1.4.2.1 2012/10/30 18:56:35 yamt Exp $	*/

/*-
 * Copyright (C) 2008 John Birrell <jb@freebsd.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible 
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * $FreeBSD: src/sys/cddl/dev/dtrace/dtrace_debug.c,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $
 *
 */

static char const hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
#define	hex2ascii(hex)	(hex2ascii_data[hex])

#ifdef DEBUG

#if defined(__amd64__)
static __inline int
dtrace_cmpset_long(volatile u_long *dst, u_long exp, u_long src)
{
	u_char res;

	__asm __volatile(
	"	 lock ; 		"
	"	cmpxchgq %2,%1 ;	"
	"       sete	%0 ;		"
	"1:				"
	"# dtrace_cmpset_long"
	: "=a" (res),			/* 0 */
	  "=m" (*dst)			/* 1 */
	: "r" (src),			/* 2 */
	  "a" (exp),			/* 3 */
	  "m" (*dst)			/* 4 */
	: "memory");

	return (res);
}
#elif defined(__i386__)
static __inline int
dtrace_cmpset_long(volatile u_long *dst, u_long exp, u_long src)
{
	u_char res;

	__asm __volatile(
	"        lock ;            	"
	"       cmpxchgl %2,%1 ;        "
	"       sete    %0 ;            "
	"1:                             "
	"# dtrace_cmpset_long"
	: "=a" (res),                   /* 0 */
	  "=m" (*dst)                   /* 1 */
	: "r" (src),                    /* 2 */
	  "a" (exp),                    /* 3 */
	  "m" (*dst)                    /* 4 */
	: "memory");

	return (res);
}
#endif

#define DTRACE_DEBUG_BUFR_SIZE	(32 * 1024)

struct dtrace_debug_data {
	char bufr[DTRACE_DEBUG_BUFR_SIZE];
	char *first;
	char *last;
	char *next;
} dtrace_debug_data[MAXCPUS];

static char dtrace_debug_bufr[DTRACE_DEBUG_BUFR_SIZE];

static volatile u_long	dtrace_debug_flag[MAXCPUS];

static void
dtrace_debug_lock(int cpu)
{
	while (dtrace_cmpset_long(&dtrace_debug_flag[cpu], 0, 1) == 0)
		/* Loop until the lock is obtained. */
		;
}

static void
dtrace_debug_unlock(int cpu)
{
	dtrace_debug_flag[cpu] = 0;
}

static void
dtrace_debug_init(void *dummy)
{
	struct dtrace_debug_data *d;
	CPU_INFO_ITERATOR cpuind;
	struct cpu_info *cinfo;

	for (CPU_INFO_FOREACH(cpuind, cinfo)) {
		d = &dtrace_debug_data[cpu_index(cinfo)];

		if (d->first == NULL) {
			d->first = d->bufr;
			d->next = d->bufr;
			d->last = d->bufr + DTRACE_DEBUG_BUFR_SIZE - 1;
			*(d->last) = '\0';
		}
	}
}

//SYSINIT(dtrace_debug_init, SI_SUB_KDTRACE, SI_ORDER_ANY, dtrace_debug_init, NULL);
//SYSINIT(dtrace_debug_smpinit, SI_SUB_SMP, SI_ORDER_ANY, dtrace_debug_init, NULL);

static void
dtrace_debug_output(void)
{
	char *p;
	struct dtrace_debug_data *d;
	uintptr_t count;
	CPU_INFO_ITERATOR cpuind;
	struct cpu_info *cinfo;
	cpuid_t cpuid;

	for (CPU_INFO_FOREACH(cpuind, cinfo)) {
	    	cpuid = cpu_index(cinfo);

		dtrace_debug_lock(cpuid);

		d = &dtrace_debug_data[cpuid];

		count = 0;

		if (d->first < d->next) {
			char *p1 = dtrace_debug_bufr;

			count = (uintptr_t) d->next - (uintptr_t) d->first;

			for (p = d->first; p < d->next; p++)
				*p1++ = *p;
		} else if (d->next > d->first) {
			char *p1 = dtrace_debug_bufr;

			count = (uintptr_t) d->last - (uintptr_t) d->first;

			for (p = d->first; p < d->last; p++)
				*p1++ = *p;

			count += (uintptr_t) d->next - (uintptr_t) d->bufr;

			for (p = d->bufr; p < d->next; p++)
				*p1++ = *p;
		}

		d->first = d->bufr;
		d->next = d->bufr;

		dtrace_debug_unlock(cpuid);

		if (count > 0) {
			char *last = dtrace_debug_bufr + count;

			p = dtrace_debug_bufr;

			while (p < last) {
				if (*p == '\0') {
					p++;
					continue;
				}

				printf("%s", p);

				p += strlen(p);
			}
		}
	}
}

/*
 * Functions below here are called from the probe context, so they can't call
 * _any_ functions outside the dtrace module without running foul of the function
 * boundary trace provider (fbt). The purpose of these functions is limited to
 * buffering debug strings for output when the probe completes on the current CPU.
 */

static __inline void
dtrace_debug__putc(char c)
{
	struct dtrace_debug_data *d = &dtrace_debug_data[cpu_number()];

	*d->next++ = c;

	if (d->next == d->last)
		d->next = d->bufr;

	*(d->next) = '\0';

	if (d->next == d->first)
		d->first++;

	if (d->first == d->last)
		d->first = d->bufr;
}

static void __used
dtrace_debug_putc(char c)
{
	dtrace_debug_lock(cpu_number());

	dtrace_debug__putc(c);

	dtrace_debug_unlock(cpu_number());
}

static void __used
dtrace_debug_puts(const char *s)
{
	dtrace_debug_lock(cpu_number());

	while (*s != '\0')
		dtrace_debug__putc(*s++);

	dtrace_debug__putc('\0');

	dtrace_debug_unlock(cpu_number());
}

/*
 * Snaffled from sys/kern/subr_prf.c
 *
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `xbuf' must have length >= MAXNBUF.
 */
static char *
dtrace_debug_ksprintn(char *xbuf, uintmax_t num, int base, int *lenp, int upper)
{
	char *p, c;

	p = xbuf;
	*p = '\0';
	do {
		c = hex2ascii(num % base);
		*++p = upper ? toupper(c) : c;
	} while (num /= base);
	if (lenp)
		*lenp = p - xbuf;
	return (p);
}

#define MAXNBUF (sizeof(intmax_t) * NBBY + 1)

static void
dtrace_debug_vprintf(const char *fmt, va_list ap)
{
	char xbuf[MAXNBUF];
	const char *p, *percent, *q;
	u_char *up;
	int ch, n;
	uintmax_t num;
	int base, lflag, qflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
	int cflag, hflag, jflag, tflag, zflag;
	int dwidth, upper;
	int radix = 10;
	char padc;
	int stop = 0, retval = 0;

	num = 0;

	if (fmt == NULL)
		fmt = "(fmt null)\n";

	for (;;) {
		padc = ' ';
		width = 0;
		while ((ch = (u_char)*fmt++) != '%' || stop) {
			if (ch == '\0') {
				dtrace_debug__putc('\0');
				return;
			}
			dtrace_debug__putc(ch);
		}
		percent = fmt - 1;
		qflag = 0; lflag = 0; ladjust = 0; sharpflag = 0; neg = 0;
		sign = 0; dot = 0; dwidth = 0; upper = 0;
		cflag = 0; hflag = 0; jflag = 0; tflag = 0; zflag = 0;
reswitch:	switch (ch = (u_char)*fmt++) {
		case '.':
			dot = 1;
			goto reswitch;
		case '#':
			sharpflag = 1;
			goto reswitch;
		case '+':
			sign = 1;
			goto reswitch;
		case '-':
			ladjust = 1;
			goto reswitch;
		case '%':
			dtrace_debug__putc(ch);
			break;
		case '*':
			if (!dot) {
				width = va_arg(ap, int);
				if (width < 0) {
					ladjust = !ladjust;
					width = -width;
				}
			} else {
				dwidth = va_arg(ap, int);
			}
			goto reswitch;
		case '0':
			if (!dot) {
				padc = '0';
				goto reswitch;
			}
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
				for (n = 0;; ++fmt) {
					n = n * 10 + ch - '0';
					ch = *fmt;
					if (ch < '0' || ch > '9')
						break;
				}
			if (dot)
				dwidth = n;
			else
				width = n;
			goto reswitch;
		case 'b':
			num = (u_int)va_arg(ap, int);
			p = va_arg(ap, char *);
			for (q = dtrace_debug_ksprintn(xbuf, num, *p++, NULL, 0); *q;)
				dtrace_debug__putc(*q--);

			if (num == 0)
				break;

			for (tmp = 0; *p;) {
				n = *p++;
				if (num & (1 << (n - 1))) {
					dtrace_debug__putc(tmp ? ',' : '<');
					for (; (n = *p) > ' '; ++p)
						dtrace_debug__putc(n);
					tmp = 1;
				} else
					for (; *p > ' '; ++p)
						continue;
			}
			if (tmp)
				dtrace_debug__putc('>');
			break;
		case 'c':
			dtrace_debug__putc(va_arg(ap, int));
			break;
		case 'D':
			up = va_arg(ap, u_char *);
			p = va_arg(ap, char *);
			if (!width)
				width = 16;
			while(width--) {
				dtrace_debug__putc(hex2ascii(*up >> 4));
				dtrace_debug__putc(hex2ascii(*up & 0x0f));
				up++;
				if (width)
					for (q=p;*q;q++)
						dtrace_debug__putc(*q);
			}
			break;
		case 'd':
		case 'i':
			base = 10;
			sign = 1;
			goto handle_sign;
		case 'h':
			if (hflag) {
				hflag = 0;
				cflag = 1;
			} else
				hflag = 1;
			goto reswitch;
		case 'j':
			jflag = 1;
			goto reswitch;
		case 'l':
			if (lflag) {
				lflag = 0;
				qflag = 1;
			} else
				lflag = 1;
			goto reswitch;
		case 'n':
			if (jflag)
				*(va_arg(ap, intmax_t *)) = retval;
			else if (qflag)
				*(va_arg(ap, quad_t *)) = retval;
			else if (lflag)
				*(va_arg(ap, long *)) = retval;
			else if (zflag)
				*(va_arg(ap, size_t *)) = retval;
			else if (hflag)
				*(va_arg(ap, short *)) = retval;
			else if (cflag)
				*(va_arg(ap, char *)) = retval;
			else
				*(va_arg(ap, int *)) = retval;
			break;
		case 'o':
			base = 8;
			goto handle_nosign;
		case 'p':
			base = 16;
			sharpflag = (width == 0);
			sign = 0;
			num = (uintptr_t)va_arg(ap, void *);
			goto number;
		case 'q':
			qflag = 1;
			goto reswitch;
		case 'r':
			base = radix;
			if (sign)
				goto handle_sign;
			goto handle_nosign;
		case 's':
			p = va_arg(ap, char *);
			if (p == NULL)
				p = "(null)";
			if (!dot)
				n = strlen (p);
			else
				for (n = 0; n < dwidth && p[n]; n++)
					continue;

			width -= n;

			if (!ladjust && width > 0)
				while (width--)
					dtrace_debug__putc(padc);
			while (n--)
				dtrace_debug__putc(*p++);
			if (ladjust && width > 0)
				while (width--)
					dtrace_debug__putc(padc);
			break;
		case 't':
			tflag = 1;
			goto reswitch;
		case 'u':
			base = 10;
			goto handle_nosign;
		case 'X':
			upper = 1;
		case 'x':
			base = 16;
			goto handle_nosign;
		case 'y':
			base = 16;
			sign = 1;
			goto handle_sign;
		case 'z':
			zflag = 1;
			goto reswitch;
handle_nosign:
			sign = 0;
			if (jflag)
				num = va_arg(ap, uintmax_t);
			else if (qflag)
				num = va_arg(ap, u_quad_t);
			else if (tflag)
				num = va_arg(ap, ptrdiff_t);
			else if (lflag)
				num = va_arg(ap, u_long);
			else if (zflag)
				num = va_arg(ap, size_t);
			else if (hflag)
				num = (u_short)va_arg(ap, int);
			else if (cflag)
				num = (u_char)va_arg(ap, int);
			else
				num = va_arg(ap, u_int);
			goto number;
handle_sign:
			if (jflag)
				num = va_arg(ap, intmax_t);
			else if (qflag)
				num = va_arg(ap, quad_t);
			else if (tflag)
				num = va_arg(ap, ptrdiff_t);
			else if (lflag)
				num = va_arg(ap, long);
			else if (zflag)
				num = va_arg(ap, size_t);
			else if (hflag)
				num = (short)va_arg(ap, int);
			else if (cflag)
				num = (char)va_arg(ap, int);
			else
				num = va_arg(ap, int);
number:
			if (sign && (intmax_t)num < 0) {
				neg = 1;
				num = -(intmax_t)num;
			}
			p = dtrace_debug_ksprintn(xbuf, num, base, &tmp, upper);
			if (sharpflag && num != 0) {
				if (base == 8)
					tmp++;
				else if (base == 16)
					tmp += 2;
			}
			if (neg)
				tmp++;

			if (!ladjust && padc != '0' && width
			    && (width -= tmp) > 0)
				while (width--)
					dtrace_debug__putc(padc);
			if (neg)
				dtrace_debug__putc('-');
			if (sharpflag && num != 0) {
				if (base == 8) {
					dtrace_debug__putc('0');
				} else if (base == 16) {
					dtrace_debug__putc('0');
					dtrace_debug__putc('x');
				}
			}
			if (!ladjust && width && (width -= tmp) > 0)
				while (width--)
					dtrace_debug__putc(padc);

			while (*p)
				dtrace_debug__putc(*p--);

			if (ladjust && width && (width -= tmp) > 0)
				while (width--)
					dtrace_debug__putc(padc);

			break;
		default:
			while (percent < fmt)
				dtrace_debug__putc(*percent++);
			/*
			 * Since we ignore an formatting argument it is no 
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			stop = 1;
			break;
		}
	}

	dtrace_debug__putc('\0');
}

void
dtrace_debug_printf(const char *fmt, ...)
{
	va_list ap;

	dtrace_debug_lock(cpu_number());

	va_start(ap, fmt);

	dtrace_debug_vprintf(fmt, ap);

	va_end(ap);

	dtrace_debug_unlock(cpu_number());
}

#else

#define dtrace_debug_output()
#define dtrace_debug_puts(_s)
#define dtrace_debug_printf(fmt, ...)

#endif
