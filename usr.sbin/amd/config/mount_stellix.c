/*
 * IRIX Mount helper
 */

#include "misc-stellix.h"

/*
 * Map from conventional mount arguments
 * to IRIX style arguments.
 */
stellix_mount(fsname, dir, flags, type, data)
char *fsname;
char *dir;
int flags;
int type;
void *data;
{

#ifdef DEBUG
	dlog("stellix_mount: fsname %s, dir %s, type %d", fsname, dir, type);
#endif /* DEBUG */

	if (type == MOUNT_TYPE_NFS) {

		return mount(dir, dir, (MS_FSS|MS_NFS|flags),
			     type, (caddr_t) data );

	} else if (type == MOUNT_TYPE_UFS) {

		return mount(fsname, dir, (MS_FSS|flags), type);

	} else {
		return EINVAL;
	}

}
