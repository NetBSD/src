/*
 * NetBSD specific device routines are added to this file.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <sys/sysctl.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>

#include "netbsd.h"

#define LVM_FAILURE -1

/*
 * Find major numbers for char/block parts of all block devices.
 * In NetBSD every block device has it's char counter part.
 * Return success only for device drivers with defined char/block
 * major numbers.
 */
int
nbsd_check_dev(int major,char *path)
{

	size_t val_len,i;

	struct kinfo_drivers *kd;

	/* XXX HACK */	
	if (strcmp(path,"/dev/console") == 0)
		return LVM_FAILURE;
	
	/* get size kernel drivers array from kernel*/
	if (sysctlbyname("kern.drivers",NULL,&val_len,NULL,0) < 0) {
		printf("sysctlbyname failed");
		return LVM_FAILURE;
	}
	
	if ((kd = malloc (val_len)) == NULL){
		printf("malloc kd info error\n");
		return LVM_FAILURE;
	}
	
	/* get array from kernel */
	if (sysctlbyname("kern.drivers", kd, &val_len, NULL, 0) < 0) {
		printf("sysctlbyname failed kd");
		return LVM_FAILURE;
	}

	for (i = 0, val_len /= sizeof(*kd); i < val_len; i++)
		/* We select only devices with correct char/block major number. */
		if (kd[i].d_cmajor != -1 && kd[i].d_bmajor != -1) {
			
			if (kd[i].d_cmajor == major)
				return kd[i].d_bmajor;
			
			if (kd[i].d_bmajor == major)
				return kd[i].d_cmajor;
			
		}
	
	return LVM_FAILURE;
}
