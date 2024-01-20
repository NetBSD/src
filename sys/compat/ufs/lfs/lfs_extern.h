/*	$NetBSD: lfs_extern.h,v 1.1 2024/01/20 16:28:43 christos Exp $	*/

#ifndef _COMPAT_UFS_LFS_LFS_EXTERN_H_
#define _COMPAT_UFS_LFS_LFS_EXTERN_H_

__BEGIN_DECLS
#ifndef _KERNEL
int lfs_segwait(fsid_t *, struct timeval50 *);
int __lfs_segwait50(fsid_t *, struct timeval *);
#endif
__END_DECLS

#endif /* !_COMPAT_UFS_LFS_LFS_EXTERN_H_ */
