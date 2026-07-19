/*	$NetBSD$	*/

#ifndef _MISCFS_FULLFS_IOCTL_H_
#define _MISCFS_FULLFS_IOCTL_H_

#include <sys/ioccom.h>
#include <sys/types.h>

#define FULLFS_MODE_PASS	0
#define FULLFS_MODE_FAIL	1
#define FULLFS_MODE_COUNT	2
#define FULLFS_MODE_BYTES	3
#define FULLFS_MODE_RANDOM	4

#define FULLFS_OP_NONE		0x00
#define FULLFS_OP_WRITE		0x01
#define FULLFS_OP_CREATE	0x02
#define FULLFS_OP_MKDIR		0x04
#define FULLFS_OP_MKNOD		0x08
#define FULLFS_OP_SYMLINK	0x10
#define FULLFS_OP_LINK		0x20
#define FULLFS_OP_ALL		0x3F

struct fullfs_ctl {
	int		fc_mode;
	int		fc_opmask;
	int		fc_error;
	unsigned int	fc_rate;
	unsigned int	fc_doom;
};

#define	FULLFS_GETSTATE	_IOR('F', 30, struct fullfs_ctl)
#define	FULLFS_SETSTATE	_IOW('F', 31, struct fullfs_ctl)

#endif /* _MISCFS_FULLFS_IOCTL_H_ */
