#	$NetBSD: Makefile,v 1.20 1995/03/18 16:15:28 cgd Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

# Not ported: XNSrouted enpload scsiformat startslip
# Missing: fsdb icheck ncheck

SUBDIR=	badsect clri disklabel dmesg dump dumpfs dumplfs fastboot fsck \
	ifconfig init mknod modload modunload mount mount_ados \
	mount_cd9660 mount_fdesc mount_kernfs mount_lfs mount_msdos mount_nfs \
	mount_null mount_portal mount_procfs mount_umap mount_union mountd \
	newfs newlfs nfsd nfsiod nologin ping quotacheck reboot restore \
	route routed savecore shutdown slattach swapon ttyflags tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+= bim fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.elif ${MACHINE} == "pc532"
SUBDIR+= bim
.endif

.include <bsd.subdir.mk>
