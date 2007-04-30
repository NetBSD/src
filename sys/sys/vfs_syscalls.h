/*     $NetBSD: vfs_syscalls.h,v 1.3 2007/04/30 08:32:14 dsl Exp $        */

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

/* VFS status functions to kernel buffers */
int do_sys_pstatvfs(struct lwp *, const char *, int, struct statvfs *);
int do_sys_fstatvfs(struct lwp *, int, int, struct statvfs *);
/* VFS status - call copyfn() for each entry */
int do_sys_getvfsstat(struct lwp *, void *, size_t, int, int (*)(const void *, void *, size_t), size_t, register_t *);

int	vfs_copyinfh_alloc(const void *, size_t, fhandle_t **);
void	vfs_copyinfh_free(fhandle_t *);

int dofhopen(struct lwp *, const void *, size_t, int, register_t *);

#endif /* _SYS_VFS_SYSCALLS_H_ */
