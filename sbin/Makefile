#	$NetBSD: Makefile,v 1.53 1998/12/28 14:03:27 hannken Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

.include <bsd.own.mk>	# get OBJECT_FMT definition

SUBDIR=	atactl badsect bim ccdconfig disklabel dmesg edlabel fastboot fdisk \
	fsck fsirand ifconfig init mknod modload modunload mount nologin ping \
	reboot raidctl rndctl route routed savecore scsictl shutdown slattach \
	swapctl ttyflags umount wsconsctl

# support for various file systems
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_ext2fs	fsck_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_filecore
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

.if (${OBJECT_FMT} != "ELF")
SUBDIR+= ldconfig
.endif

.include <bsd.subdir.mk>
