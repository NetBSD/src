#	from: @(#)Makefile	5.4.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.17 1994/06/03 00:35:22 chopps Exp $

# Not ported: XNSrouted enpload
# Missing: icheck ncheck

SUBDIR=	badsect clri disklabel dmesg dump dumpfs fastboot fsck \
	halt ifconfig init mknod modload modunload mount mount_ados \
	mount_fdesc mount_isofs mount_kernfs mount_lofs mount_msdos \
	mount_portal mount_procfs mountd newfs \
	nfsd nfsiod ping quotacheck reboot restore route routed savecore \
	shutdown slattach swapon ttyflags tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+= bim fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.elif ${MACHINE} == "pc532"
SUBDIR+= bim
.endif

.include <bsd.subdir.mk>
