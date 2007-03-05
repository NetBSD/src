# $NetBSD: Makefile,v 1.18 2007/03/05 23:18:06 dillo Exp $

.include <bsd.own.mk>

SUBDIR+=	adosfs
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	filecorefs
SUBDIR+=	hfsp
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
