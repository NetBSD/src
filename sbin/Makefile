#	$NetBSD: Makefile,v 1.22 1995/08/17 16:38:53 thorpej Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: fsdb icheck ncheck

SUBDIR=	badsect disklabel ccdconfig dmesg fastboot ifconfig init mknod \
	modload modunload mount mountd nfsd nfsiod nologin ping quotacheck \
	reboot route routed savecore shutdown slattach swapon ttyflags umount

# support for various file systems
SUBDIR+= mount_ados
SUBDIR+= mount_cd9660
SUBDIR+= mount_fdesc
SUBDIR+= mount_ffs	newfs fsck dumpfs dump restore clri tunefs
SUBDIR+= mount_kernfs
SUBDIR+= mount_lfs	newlfs dumplfs
# mount_mfs -> newfs
SUBDIR+= mount_msdos
SUBDIR+= mount_nfs
SUBDIR+= mount_null
SUBDIR+= mount_portal
SUBDIR+= mount_procfs
SUBDIR+= mount_umap
SUBDIR+= mount_union

.if   make(clean) || make(cleandir)
SUBDIR+= bim fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.elif ${MACHINE} == "pc532"
SUBDIR+= bim
.endif

.include <bsd.subdir.mk>
