/*	$NetBSD: procfs_machdep.c,v 1.3.112.1 2014/05/22 11:39:50 yamt Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.3.112.1 2014/05/22 11:39:50 yamt Exp $");

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
procfs_getcpuinfstr(char *buf, size_t *len)
{
	*len = 0;

	return 0;
}
