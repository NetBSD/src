#ifndef _LFS_KERNEL_OPS_H_
#define _LFS_KERNEL_OPS_H_

#include <sys/types.h>
#include <sys/statvfs.h>

#include <fcntl.h>
#include <unistd.h>

struct kernelops {
	int (*ko_open)(const char *, int, mode_t);
	int (*ko_statvfs)(const char *, struct statvfs *, int);
	int (*ko_fcntl)(int, int, void *);
	int (*ko_fhopen)(const void *, size_t, int);
	int (*ko_close)(int);

	ssize_t (*ko_pread)(int, void *, size_t, off_t);
	ssize_t (*ko_pwrite)(int, const void *, size_t, off_t);
};
extern struct kernelops kops;

#endif /* _LFS_KERNEL_OPS_H_ */
