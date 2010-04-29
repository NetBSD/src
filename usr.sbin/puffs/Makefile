#	$NetBSD: Makefile,v 1.14 2010/04/29 22:42:48 pooka Exp $

SUBDIR=	mount_9p mount_psshfs mount_sysctlfs

SUBDIR+=rump_cd9660 rump_efs rump_ext2fs rump_ffs rump_hfs rump_lfs
SUBDIR+=rump_msdos rump_nfs rump_ntfs rump_smbfs rump_syspuffs rump_sysvbfs
SUBDIR+=rump_tmpfs rump_udf

#
# The following are not built by default
#

# userspace fdesc server cannot grope the fd info from the kernel,
# and is therefore not useful for the average user
#SUBDIR+=rump_fdesc

# ditto for kernfs
#SUBDIR+=rump_kernfs

# rump_mqmfs is just another ffs, useful mainly for debugging
#SUBDIR+=rump_nqmfs

# rump_au-naturel presents lets you access '/' for a rump kernel.
# mostly for debugging.
#SUBDIR+=rump_au-naturel

.include <bsd.subdir.mk>
