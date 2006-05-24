/*	$NetBSD: procfs_machdep.c,v 1.4.12.1 2006/05/24 15:48:15 tron Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.4.12.1 2006/05/24 15:48:15 tron Exp $");

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
procfs_getcpuinfstr(char *buf, int *len)
{

	*len = 0;

	return 0;
}
