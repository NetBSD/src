#ifndef EXFATFS_MOUNT_H_
#define EXFATFS_MOUNT_H_

#define	MPTOXMP(mp)	((struct exfatfs_mount *)(mp)->mnt_data)
#define	XMPTOMP(xmp)	((xmp)->xm_mp)

#include <sys/types.h>

struct exfatfs_mount {
	struct mount *xm_mp;
	u_int32_t xm_flags;
#define EXFATFS_MNTOPT		0x0
#define	EXFATFSMNT_RONLY	0x80000000	/* mounted read-only	*/
#define	EXFATFSMNT_SYNC		0x40000000	/* mounted synchronous	*/
	struct exfatfs *xm_fs;
};

#define EXFATFSMNT_MNTOPT 0x0

#endif /* EXFATFS_MOUNT_H_ */
