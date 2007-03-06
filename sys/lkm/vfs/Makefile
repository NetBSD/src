# $NetBSD: Makefile,v 1.19 2007/03/06 11:28:45 dillo Exp $

.include <bsd.own.mk>

SUBDIR+=	adosfs
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
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
