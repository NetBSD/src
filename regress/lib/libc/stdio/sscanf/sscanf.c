#include <stdio.h>
#include <err.h>

static const char str[] = "\f\n\r\t\v%z";
int
main(void)
{
        /* set of "white space" symbols from isspace(3) */
        char c = 0;
        (void)sscanf(str, "%%%c", &c);
	if (c != 'z')
		errx(1, "expected `%c', got `%c'", 'z', c);
        return 0;
}
