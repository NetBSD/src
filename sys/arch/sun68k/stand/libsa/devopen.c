/*	$NetBSD: devopen.c,v 1.1 2001/06/14 12:57:14 fredette Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

#include <stand.h>
#include "libsa.h"

/*
 * Open the device named by the combined device/file name
 * given as the "fname" arg, something like: "sd()netbsd"
 *
 * However, Sun PROMs don't really let you choose which
 * device you will talk to.  You can only open the device
 * that was used to load the boot program.  Therefore, we
 * do not accept a "device" part in the "fname" string.
 * Pass the PROM device name to open in case it needs it.
 */
int
devopen(f, fname, file)
	struct open_file *f;
	const char *fname;
	char **file;
{
	struct devsw *dp;
	int error;

	*file = (char*)fname;
	dp = &devsw[0];
	f->f_dev = dp;
	error = (*dp->dv_open)(f, prom_bootdev);

	return (error);
}
