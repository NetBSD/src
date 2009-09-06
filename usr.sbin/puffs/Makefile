#	$NetBSD: Makefile,v 1.10 2009/09/06 21:03:27 pooka Exp $

SUBDIR=	mount_9p mount_portal mount_psshfs mount_sysctlfs

SUBDIR+=rump_cd9660 rump_efs rump_ext2fs rump_ffs rump_hfs rump_lfs
SUBDIR+=rump_msdos rump_nfs rump_ntfs rump_smbfs rump_syspuffs rump_sysvbfs
SUBDIR+=rump_tmpfs rump_udf

#
# The following are not built by default
#

# userspace fdesc server cannot grope the fd info from the kernel,
# and is therefore not useful for the average user
#SUBDIR+=rump_fdesc

.include <bsd.subdir.mk>
