#	$NetBSD: Makefile,v 1.74 2001/08/17 21:43:55 thorpej Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: icheck ncheck

SUBDIR=	atactl badsect bim brconfig ccdconfig disklabel dmesg edlabel \
	fastboot fdisk fsck fsirand ifconfig init ldconfig lmcctl \
	mbrlabel mknod modload modunload mount newbtconf nologin ping \
	raidctl reboot rcorder rndctl route routed savecore scsictl \
	shutdown slattach swapctl sysctl ttyflags umount wdogctl wsconsctl

# support for various file systems
SUBDIR+= fsck_ext2fs
SUBDIR+= newfs fsck_ffs fsdb dump restore clri tunefs
SUBDIR+= newfs_lfs fsck_lfs dump_lfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= mount_portal

# following is now build into mount(8) now (see mount/Makefile)
# we only dive into those subdirs for make obj/clean - some arch's floppies
# compile individual mount_* and hence need the obj dirs to allow shared
# source tree
.if make(obj) || make(clean) || make(distclean) || make(cleandir)
SUBDIR+= mount_ados mount_cd9660 mount_ext2fs mount_fdesc mount_filecore \
	   mount_ffs mount_kernfs mount_lfs mount_msdos mount_nfs 	\
	   mount_ntfs mount_null mount_overlay mount_procfs mount_umap	\
	   mount_union
.endif

# IPv6
SUBDIR+= ping6 rtsol

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
