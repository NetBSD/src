#	$NetBSD: Makefile,v 1.21.4.2 2025/08/02 05:58:54 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	mount_9p mount_psshfs mount_sysctlfs

.if (${MKRUMP} != "no")
SUBDIR+=rump_cd9660 rump_efs rump_ext2fs rump_ffs rump_hfs rump_lfs
SUBDIR+=rump_msdos rump_nfs rump_ntfs rump_syspuffs rump_sysvbfs
SUBDIR+=rump_tmpfs rump_udf rump_v7fs

.if (${MKEXFATFS} != "no")
SUBDIR+=rump_exfatfs
.endif

#
# The following are not built by default
#

# userspace fdesc server cannot grope the fd info from the kernel,
# and is therefore not useful for the average user
#SUBDIR+=rump_fdesc

# ditto for kernfs
#SUBDIR+=rump_kernfs

# rump_nqmfs is just another ffs, useful mainly for debugging
#SUBDIR+=rump_nqmfs

# rump_au-naturel lets you access '/' for a rump kernel.
# mostly for debugging.
#SUBDIR+=rump_au-naturel

.endif

.include <bsd.subdir.mk>
