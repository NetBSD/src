#	$NetBSD: Makefile,v 1.44 1997/07/23 07:16:15 mikel Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

SUBDIR=	badsect bim ccdconfig disklabel dmesg edlabel fastboot fdisk fsck \
	fsirand ifconfig init mknod modload modunload mount nologin ping \
	reboot route routed savecore shutdown slattach swapctl ttyflags \
	umount

# support for various file systems
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_ext2fs	fsck_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_ffs	newfs fsck_ffs fsdb dump restore clri tunefs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs	newlfs
# mount_mfs -> newfs
SUBDIR+= mount_msdos	newfs_msdos fsck_msdos
SUBDIR+= mount_nfs
SUBDIR+= mount_null
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

.include <bsd.subdir.mk>
