#	from: @(#)Makefile	5.4.1.1 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.14 1994/01/12 20:24:12 cgd Exp $

# Not ported: XNSrouted enpload
# Missing: icheck ncheck

SUBDIR=	badsect clri disklabel dmesg dump dumpfs fastboot fsck \
	halt ifconfig init mknod modload modunload mount mount_fdesc \
	mount_isofs mount_kernfs mount_lofs mount_msdos mount_portal \
	mount_procfs mountd newfs \
	nfsd nfsiod ping quotacheck reboot restore route routed savecore \
	shutdown slattach swapon tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+= fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.endif

.include <bsd.subdir.mk>
