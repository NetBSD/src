# $NetBSD: Makefile,v 1.20 2007/06/29 23:30:31 rumble Exp $

.include <bsd.own.mk>

SUBDIR+=	adosfs
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	efs
SUBDIR+=	filecorefs
SUBDIR+=	hfs
SUBDIR+=	miscfs
SUBDIR+=	msdosfs
SUBDIR+=	ntfs
SUBDIR+=	ptyfs
SUBDIR+=	smbfs
SUBDIR+=	tmpfs
SUBDIR+=	ufs
SUBDIR+=	udf

.if (${MKPUFFS} != "no")
SUBDIR+=	puffs
.endif

.include <bsd.subdir.mk>
