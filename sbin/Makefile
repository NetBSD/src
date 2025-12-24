#	$NetBSD: Makefile,v 1.143 2025/12/24 15:37:29 thorpej Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

.include <bsd.own.mk>

SUBDIR=	amrctl apmlabel atactl badsect bioctl blkdiscard brconfig \
	canconfig ccdconfig cgdconfig chown \
	devpubd disklabel dkctl dkscan_bsdlabel dmesg dmctl drvctl \
	fastboot fdisk fsck fsirand gpt ifconfig init ldconfig luactl \
	mbrlabel mknod modload modstat modunload mount \
	newbtconf nologin nvmectl \
	ping pppoectl raidctl reboot rcorder rndctl route routed \
	savecore scsictl shutdown slattach svhlabel swapctl sysctl \
	ttyflags umbctl umount veriexecctl wdogctl wsconsctl

.if ${HAVE_EFI_RT} == "yes"
SUBDIR+= efi
.endif

#
# support for various file systems
#
# Always include FFS.
#
SUBDIR+= newfs fsck_ffs fsdb dump restore clri tunefs resize_ffs mount_ffs \
	scan_ffs

.if (${MKADOSFS} != "no")
SUBDIR+= mount_ados
.endif

.if (${MKAUTOFS} != "no")
SUBDIR+= mount_autofs
.endif

.if (${MKCD9660FS} != "no")
SUBDIR+= mount_cd9660
.endif

.if (${MKCHFS} != "no")
SUBDIR+= mount_chfs
.endif

.if (${MKEFS} != "no")
SUBDIR+= mount_efs
.endif

.if (${MKEXT2FS} != "no")
SUBDIR+= newfs_ext2fs fsck_ext2fs mount_ext2fs
.endif

.if (${MKFDESCFS} != "no")
SUBDIR+= mount_fdesc
.endif

.if (${MKFILECOREFS} != "no")
SUBDIR+= mount_filecore
.endif

.if (${MKHFS} != "no")
SUBDIR+= mount_hfs
.endif

.if (${MKKERNFS} != "no")
SUBDIR+= mount_kernfs
.endif

.if (${MKLFS} != "no")
SUBDIR+= newfs_lfs fsck_lfs dump_lfs resize_lfs mount_lfs
.endif

.if (${MKMSDOSFS} != "no")
SUBDIR+= newfs_msdos fsck_msdos mount_msdos
.endif

.if (${MKNFS} != "no")
SUBDIR+= mount_nfs
.endif

.if (${MKNILFS} != "no")
SUBDIR+= mount_nilfs
.endif

.if (${MKNTFS} != "no")
SUBDIR+= mount_ntfs
.endif

.if (${MKNULLFS} != "no")
SUBDIR+= mount_null
.endif

.if (${MKOVERLAYFS} != "no")
SUBDIR+= mount_overlay
.endif

# XXX De-tangle this web (puffs) later.
SUBDIR+= mount_portal

.if (${MKPROCFS} != "no")
SUBDIR+= mount_procfs
.endif

.if (${MKPTYFS} != "no")
SUBDIR+= mount_ptyfs
.endif

# XXX De-tangle this web (puffs) later.
SUBDIR+= mount_puffs

.if (${MKQEMUFWCFG} != "no")
SUBDIR+= mount_qemufwcfg
.endif

.if (${MKSYSVBFS} != "no")
SUBDIR+= newfs_sysvbfs mount_sysvbfs
.endif

.if (${MKTMPFS} != "no")
SUBDIR+= mount_tmpfs
.endif

.if (${MKUDF} != "no")
SUBDIR+= newfs_udf fsck_udf mount_udf
.endif

.if (${MKUMAPFS} != "no")
SUBDIR+= mount_umap
.endif

.if (${MKUNIONFS} != "no")
SUBDIR+= mount_union
.endif

.if (${MKV7FS} != "no")
SUBDIR+= newfs_v7fs fsck_v7fs mount_v7fs
.endif

.if (${USE_INET6} != "no")
SUBDIR+= ping6
.endif

.if (${MKISCSI} != "no")
SUBDIR+= iscsictl iscsid
.endif

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
