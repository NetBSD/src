#	@(#)Makefile	5.4.1.1 (Berkeley) 5/7/91
#
#	$Header: /cvsroot/src/sbin/Makefile,v 1.7 1993/04/09 19:23:22 cgd Exp $

SUBDIR=	badsect chown clri disklabel dmesg dump dumpfs fastboot fsck \
	halt ifconfig init mknod mount mount_isofs mount_miscfs mount_pcfs \
	mountd newfs nfsd nfsiod ping quotacheck reboot restore route \
	routed savecore shutdown slattach swapon tunefs umount

.if   make(clean) || make(cleandir)
SUBDIR+= fdisk
.elif ${MACHINE} == "i386"
SUBDIR+= fdisk
.endif

+ 
+ # Not ported: XNSrouted enpload
+ # Missing: icheck ncheck

.include <bsd.subdir.mk>
