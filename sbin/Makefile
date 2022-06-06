#	$NetBSD: Makefile,v 1.138 2022/06/06 10:56:28 nia Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

.include <bsd.own.mk>

SUBDIR=	amrctl apmlabel atactl badsect bioctl blkdiscard brconfig \
	canconfig ccdconfig cgdconfig chown \
	devpubd disklabel dkctl dkscan_bsdlabel dmesg dmctl \
	drvctl fastboot fdisk fsck fsirand gpt ifconfig init ldconfig luactl \
	mbrlabel mknod modload modstat modunload mount \
	newbtconf nologin nvmectl \
	ping pppoectl raidctl reboot rcorder rndctl route routed \
	savecore scan_ffs scsictl shutdown slattach svhlabel swapctl sysctl \
	ttyflags umbctl umount veriexecctl wdogctl wsconsctl

# support for various file systems
SUBDIR+= newfs_ext2fs fsck_ext2fs
SUBDIR+= newfs fsck_ffs fsdb dump restore clri tunefs resize_ffs
SUBDIR+= newfs_lfs fsck_lfs dump_lfs resize_lfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= newfs_udf fsck_udf
SUBDIR+= mount_autofs
SUBDIR+= mount_cd9660
SUBDIR+= mount_ext2fs
SUBDIR+= mount_fdesc
SUBDIR+= mount_ffs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs
SUBDIR+= mount_msdos
SUBDIR+= mount_udf
SUBDIR+= mount_nfs
SUBDIR+= mount_ntfs
SUBDIR+= mount_null
SUBDIR+= mount_overlay
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_ptyfs
SUBDIR+= mount_puffs
SUBDIR+= mount_qemufwcfg
SUBDIR+= mount_tmpfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

.if (${USE_INET6} != "no")
SUBDIR+= ping6
.endif

.if (${MKISCSI} != "no")
SUBDIR+= iscsictl iscsid
.endif

.if (${MKADOS} != "no")
SUBDIR+= mount_ados
.endif

.if (${MKCHFS} != "no")
SUBDIR+= mount_chfs
.endif

.if (${MKEFS} != "no")
SUBDIR+= mount_efs
.endif

.if (${MKFILECORE} != "no")
SUBDIR+= mount_filecore
.endif

.if (${MKHFS} != "no")
SUBDIR+= mount_hfs
.endif

.if (${MKLFS} != "no")
SUBDIR+= newfs_lfs fsck_lfs dump_lfs resize_lfs
SUBDIR+= mount_lfs
.endif

.if (${MKNILFS} != "no")
SUBDIR+= mount_nilfs
.endif

.if (${MKSYSVBFS} != "no")
SUBDIR+= newfs_sysvbfs
SUBDIR+= mount_sysvbfs
.endif

.if (${MKV7FS} != "no")
SUBDIR+= newfs_v7fs fsck_v7fs
SUBDIR+= mount_v7fs
.endif

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
