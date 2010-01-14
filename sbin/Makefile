#	$NetBSD: Makefile,v 1.115 2010/01/14 21:29:26 pooka Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

.include <bsd.own.mk>

SUBDIR=	amrctl apmlabel atactl badsect bioctl brconfig ccdconfig \
	disklabel dkctl dkscan_bsdlabel dmesg \
	drvctl edlabel fastboot fdisk fsck fsirand gpt ifconfig init ldconfig \
	mbrlabel mknod modload modstat modunload mount newbtconf nologin \
	pdisk ping pppoectl raidctl reboot rcorder rndctl route routed \
	savecore scan_ffs scsictl shutdown slattach svhlabel swapctl sysctl \
	ttyflags umount veriexecctl wdogctl wsconsctl

# support for various file systems
SUBDIR+= newfs_ext2fs fsck_ext2fs
SUBDIR+= newfs fsck_ffs fsdb dump restore clri tunefs
SUBDIR+= newfs_lfs fsck_lfs dump_lfs resize_lfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= newfs_sysvbfs
SUBDIR+= newfs_udf
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_efs
SUBDIR+= mount_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_filecore
SUBDIR+= mount_ffs
SUBDIR+= mount_hfs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs
SUBDIR+= mount_msdos
SUBDIR+= mount_udf
SUBDIR+= mount_nfs
SUBDIR+= mount_nilfs
SUBDIR+= mount_ntfs
SUBDIR+= mount_null
SUBDIR+= mount_overlay
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_ptyfs
SUBDIR+= mount_puffs
SUBDIR+= mount_smbfs
SUBDIR+= mount_sysvbfs
SUBDIR+= mount_tmpfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

.if (${MKCRYPTO} != "no")
SUBDIR+= cgdconfig
.endif

.if (${USE_INET6} != "no")
SUBDIR+= ping6 rtsol
.endif

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
