/*	$NetBSD: compat_mount.c,v 1.3 2024/01/20 14:52:46 christos Exp $	*/


#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat_mount.c,v 1.3 2024/01/20 14:52:46 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include <sys/types.h>
#include <sys/mount.h>
#include <compat/sys/mount.h>

__warn_references(mount,
    "warning: reference to compatibility mount(); include <sys/mount.h> to generate correct reference")

/*
 * Convert old mount() call to new calling convention
 * The kernel will treat length 0 as 'default for the fs'.
 * We need to throw away the +ve response for MNT_GETARGS.
 */
int
mount(const char *type, const char *dir, int flags, void *data)
{
	return __mount50(type, dir, flags, data, 0) == -1 ? -1 : 0;
}
