#include <stdio.h>
#include <dlfcn.h>
#include <err.h>

int main(void)
{
	void *handle, *sym;
	char *error;

	/*
	 * Test for dlerror() being set by a successful library open.
	 * Requires that the rpath be set to something that does not
	 * include libm.so.
	 */

	handle = dlopen("libm.so", RTLD_LAZY);
	error = dlerror();
	if (error != NULL)
		errx(1, "Error opening libm.so: %s", error);
	if (handle == NULL)
		errx(1, "Library handle is NULL but dlerror not set.");

	sym = dlsym(handle, "sin");
	error = dlerror();
	if (error != NULL)
		errx(1, "Error looking up sin(): %s", error);
	if (sym == NULL)
		errx(1, "Looked-up symbol is NULL but dlerror not set.");

	dlclose(handle);
	error = dlerror();
	if (error != NULL)
		errx(1, "Error calling dlclose(): %s", error);

	printf("OK\n");

	return 0;
}
