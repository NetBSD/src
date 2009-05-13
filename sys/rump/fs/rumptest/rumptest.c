#include <sys/types.h>

#include <rump/rump.h>

#include <stdio.h>
#include <string.h>

int
main(void)
{
	int rv;

	rv = rump_init();
	printf("rump init returned: %d (%s)\n", rv,
	    rv == 0 ? "success" : strerror(rv));
}
