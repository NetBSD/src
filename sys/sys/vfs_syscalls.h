/*     $NetBSD: vfs_syscalls.h,v 1.10 2009/07/02 12:53:47 pooka Exp $        */

#ifndef _SYS_VFS_SYSCALLS_H_
#define _SYS_VFS_SYSCALLS_H_

#include <sys/types.h>
#include <sys/fstypes.h>

struct stat;
struct statvfs;

extern int dovfsusermount;

/*
 * syscall helpers for compat code.
 */

/* Status functions to kernel 'struct stat' buffers */
int do_sys_stat(const char *, unsigned int, struct stat *);
int do_fhstat(struct lwp *, const void *, size_t, struct stat *);
int do_fhstatvfs(struct lwp *, const void *, size_t, struct statvfs *, int);

/* VFS status functions to kernel buffers */
int do_sys_pstatvfs(struct lwp *, const char *, int, struct statvfs *);
int do_sys_fstatvfs(struct lwp *, int, int, struct statvfs *);
/* VFS status - call copyfn() for each entry */
int do_sys_getvfsstat(struct lwp *, void *, size_t, int, int (*)(const void *, void *, size_t), size_t, register_t *);

int do_sys_utimes(struct lwp *, struct vnode *, const char *, int,
    const struct timeval *, enum uio_seg);

int	vfs_copyinfh_alloc(const void *, size_t, fhandle_t **);
void	vfs_copyinfh_free(fhandle_t *);

int dofhopen(struct lwp *, const void *, size_t, int, register_t *);

int	do_sys_unlink(const char *, enum uio_seg);
int	do_sys_rename(const char *, const char *, enum uio_seg, int);
int	do_sys_mknod(struct lwp *l, const char *, mode_t, dev_t, register_t *);
int	do_sys_mkdir(const char *, mode_t);

#endif /* _SYS_VFS_SYSCALLS_H_ */
