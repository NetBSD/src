#	from: @(#)Makefile	5.4.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.18 1994/06/08 19:40:22 mycroft Exp $

# Not ported: XNSrouted enpload
# Missing: icheck ncheck

SUBDIR=	badsect clri disklabel dmesg dump dumpfs dumplfs fastboot fsck \
	halt ifconfig init mknod modload modunload mount mount_ados \
	mount_cd9660 mount_fdesc mount_kernfs mount_lfs mount_msdos mount_nfs \
	mount_null mount_portal mount_procfs mount_umap mount_union mountd \
	newfs newlfs nfsd nfsiod ping quotacheck reboot restore route routed \
	savecore shutdown slattach swapon ttyflags tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+= bim fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.elif ${MACHINE} == "pc532"
SUBDIR+= bim
.endif

.include <bsd.subdir.mk>
