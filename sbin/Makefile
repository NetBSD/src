#	$NetBSD: Makefile,v 1.83 2003/02/18 20:35:30 jdolecek Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

SUBDIR=	atactl badsect bim brconfig ccdconfig cgdconfig disklabel dkctl \
	dmesg edlabel fastboot fdisk fsck fsirand ifconfig init ldconfig \
	lmcctl mbrlabel mknod modload modunload mount newbtconf nologin \
	pdisk ping pppoectl raidctl reboot rcorder rndctl route routed \
	savecore scsictl shutdown slattach swapctl sysctl ttyflags umount \
	veriexecctl wdogctl wsconsctl

# support for various file systems
SUBDIR+= fsck_ext2fs
SUBDIR+= newfs fsck_ffs fsdb dump restore clri tunefs
SUBDIR+= newfs_lfs fsck_lfs dump_lfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_filecore
SUBDIR+= mount_ffs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs
SUBDIR+= mount_msdos
SUBDIR+= mount_nfs
SUBDIR+= mount_ntfs
SUBDIR+= mount_null
SUBDIR+= mount_overlay
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_smbfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

# IPv6
SUBDIR+= ping6 rtsol

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
