#include <lib/libsa/stand.h>

/* XXX non-standard */

void
puts(s)
	char *s;
{

	while (*s)
		putchar(*s++);
}
