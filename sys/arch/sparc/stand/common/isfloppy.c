#include <lib/libkern/libkern.h>
#include <sparc/stand/common/isfloppy.h>

int
bootdev_isfloppy(const char *dev)
{
	return strncmp(dev, "fd", 2) == 0 ||
		strstr(dev, "SUNW,fdtwo") != NULL ||
		strstr(dev, "fdthree") != NULL;
}
