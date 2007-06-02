/* $NetBSD: printf.c,v 1.1.2.1 2007/06/02 08:44:39 nisimura Exp $ */

/*
 * printf -- format and write output using 'func' to write characters
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <machine/stdarg.h>

#define MAXSTR	80

static int _doprnt(int (*)(int), const char *, va_list);
static void pr_int();
static int sputchar(int);
extern int putchar(int);

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
vprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_doprnt(putchar, fmt, ap);
	va_end(ap);
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
_doprnt(func, fmt, ap)
	int (*func)(int);	/* Function to put a character */
	const char *fmt;	/* Format string for pr_int/pr_float */
	va_list ap;		/* Arguments to pr_int/pr_float */
{
	int i;
	char *str;
	char string[20];
	int length;
	int leftjust;
	int longflag;
	int fmax, fmin;
	int leading;
	int outcnt;
	char fill;
	char sign;

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
		longflag = (*fmt == 'l');
		if (longflag)
			fmt++;
		if ((i = *fmt++) == '\0') {
			(*func)('%');
			outcnt += 1;
			break;
		}
		str = string;
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
		      {
			long l = va_arg(ap, long);
			if (l < 0) { sign = '-' ; l = -l; }
			pr_int((unsigned long)l, 10, str);
		      }
			break;

		case 'u':
			pr_int(va_arg(ap, unsigned long), 10, str);
			break;

		case 'o':
			pr_int(va_arg(ap, unsigned long), 8, str);
			fmax = 0;
			break;

		case 'X':
		case 'x':
			pr_int(va_arg(ap, unsigned long), 16, str);
			fmax = 0;
			break;

		case 'p':
			pr_int(va_arg(ap, unsigned long), 16, str);
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

static void pr_int(lval, base, s)
	unsigned long lval;
	int base;
	char *s;
{
	char ptmp[12];	/* unsigned long requires 11 digit in octal form */
	int i;
	char *t = ptmp;
	static const char hexdigit[] = "0123456789abcdef";

	i = 1;
	*t++ = '\0';
	do {
		*t++ = hexdigit[lval % base];
	} while ((lval /= base) != 0 && ++i < sizeof(ptmp));
	while ((*s++ = *--t) != '\0')
		;
}

static int
sputchar(c)
	int c;
{

	if (sbuf < ebuf)
		*sbuf++ = c;
	return c;
}
