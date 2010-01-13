#include <stdio.h>
#include <dlfcn.h>
#include <err.h>

int main(void)
{
	void *handle;
	char *error;

	/*
	 * Test that an error set by dlopen() persists past a successful
	 * dlopen() call.
	 */
	handle = dlopen("libnonexistent.so", RTLD_LAZY);
	handle = dlopen("libm.so", RTLD_NOW);
	error = dlerror();
	if (error == NULL)
		errx(1, "Failed: dlerror() was cleared by successful dlopen()\n");
	printf("OK: %s\n", error);
	
	return 0;
}
