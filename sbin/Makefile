#	$NetBSD: Makefile,v 1.64 1999/11/23 05:28:16 mrg Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

SUBDIR=	atactl badsect bim ccdconfig disklabel dmesg edlabel fastboot fdisk \
	fsck fsirand ifconfig init ldconfig lmcctl mknod modload modunload \
	mount newbtconf nologin ping raidctl reboot rcorder rndctl route \
	routed savecore scsictl shutdown slattach swapctl ttyflags umount \
	wsconsctl

# support for various file systems
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_ext2fs	fsck_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_filecore
SUBDIR+= mount_ffs	newfs fsck_ffs fsdb dump restore clri tunefs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs	newfs_lfs fsck_lfs dump_lfs
# mount_mfs -> newfs
SUBDIR+= mount_msdos	newfs_msdos fsck_msdos
SUBDIR+= mount_nfs
SUBDIR+= mount_ntfs
SUBDIR+= mount_null
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

# IPv6
SUBDIR+= ping6 rtsol

.include <bsd.subdir.mk>
