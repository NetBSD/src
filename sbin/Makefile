#	$NetBSD: Makefile,v 1.41 1997/06/20 20:18:35 pk Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

SUBDIR=	badsect disklabel ccdconfig dmesg fastboot fsck fsirand ifconfig init \
	mknod modload modunload mount nologin ping reboot route \
	routed savecore shutdown slattach swapctl ttyflags umount

# support for various file systems
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
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
SUBDIR+= mount_ext2fs fsck_ext2fs

.if   make(clean) || make(cleandir)
SUBDIR+= bim edlabel fdisk
.elif ${MACHINE} == "i386" || ${MACHINE} == "powerpc"
SUBDIR+= fdisk
.elif ${MACHINE} == "pc532"
SUBDIR+= bim
.elif (${MACHINE} == "sun3") || (${MACHINE} == "atari") || \
      (${MACHINE} == "sparc")
SUBDIR+= edlabel
.endif

.include <bsd.subdir.mk>
