#	$NetBSD: Makefile,v 1.3 2005/09/29 21:06:43 jmmv Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	fstab.cdrom fstab.nfs.1 fstab.nfs.2 fstab.nfs.3 fstab.pseudo fstab.ra \
	fstab.ramdisk fstab.rd fstab.sd0.1 fstab.sd0.2 fstab.sd0.amiga \
	fstab.sd0.atari fstab.tmpfs fstab.wd0.1 fstab.wd0.2 fstab.wd0.3 \
	fstab.wd0.cobalt
FILESDIR=/usr/share/examples/fstab
.endif

.include <bsd.prog.mk>
