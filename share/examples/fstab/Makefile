#	$NetBSD: Makefile,v 1.1 2002/05/04 19:44:34 jdolecek Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	fstab.md fstab.nfs.1 fstab.nfs.2 fstab.nfs.3 fstab.ra fstab.rd \
	fstab.sd0.1 fstab.sd0.2 fstab.sd0.amiga fstab.sd0.atari fstab.wd0.1 \
	fstab.wd0.2 fstab.wd0.3 fstab.wd0.cobalt
FILESDIR=/usr/share/examples/fstab
.endif

.include <bsd.prog.mk>
