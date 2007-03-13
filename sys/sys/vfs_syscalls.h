/*     $NetBSD: vfs_syscalls.h,v 1.1.2.1 2007/03/13 16:52:06 ad Exp $        */

#ifndef _SYS_VFS_SYSCALLS_H_
#define _SYS_VFS_SYSCALLS_H_

struct stat;
struct statvfs;

/*
 * syscall helpers for compat code.
 */

/* Status functions to kernel 'struct stat' buffers */
int do_sys_stat(struct lwp *, const char *, unsigned int, struct stat *);
int do_fhstat(struct lwp *, const void *, size_t, struct stat *);
int do_fhstatvfs(struct lwp *, const void *, size_t, struct statvfs *, int);

int	vfs_copyinfh_alloc(const void *, size_t, fhandle_t **);
void	vfs_copyinfh_free(fhandle_t *);

int dofhopen(struct lwp *, const void *, size_t, int, register_t *);
/* These 2 have user-space stat buffer pointer and must be killed */
int dofhstat(struct lwp *, const void *, size_t, struct stat *, register_t *);
int dofhstatvfs(struct lwp *, const void *, size_t, struct statvfs *, int,
    register_t *);

#endif /* _SYS_VFS_SYSCALLS_H_ */
