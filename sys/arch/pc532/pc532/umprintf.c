#include <stdarg.h>
static char *ksprintn __P((u_long num, int base, int *len));

void
umprintf(char *fmt,...)
{
	va_list ap;
	char *p;
	int tmp;
	int base;
	unsigned long ul;
	char ch;

        va_start (ap,fmt);

	for (;;) {
		while ((ch = *fmt++) != '%') {
			if (ch == '\0')
				return;
			scncnputc(0, ch);
		}
		ch = *fmt++;
		switch (ch) {
		case 'd':
			ul = va_arg(ap, u_long);
			base = 10;
			goto number;
		case 'x':
			ul = va_arg(ap, u_long);
			base = 16;
number:			p = ksprintn(ul, base, &tmp);
			while (ch = *p--)
				scncnputc(0,ch);
			break;
		default:
			scncnputc(0,ch);
		}
	}
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
ksprintn(ul, base, lenp)
	register u_long ul;
	register int base, *lenp;
{					/* A long in base 8, plus NULL. */
	static char buf[sizeof(long) * NBBY / 3 + 2];
	register char *p;
	int d;

	p = buf;
	*p='\0';
	do {
		d = ul % base;
		if (d < 10)
			*++p = '0' + d;
		else
			*++p = 'a' + d - 10;
	} while (ul /= base);
	if (lenp)
		*lenp = p - buf;
	return (p);
}
