/*	$NetBSD: procfs_machdep.c,v 1.2.24.4 2005/11/10 13:59:34 skrll Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.2.24.4 2005/11/10 13:59:34 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <miscfs/procfs/procfs.h>


/*
 * Linux-style /proc/cpuinfo.
 * Only used when procfs is mounted with -o linux.
 */
int
procfs_getcpuinfstr(char *sbuf, int *len)
{
	*len = 0;

	return 0;
}
