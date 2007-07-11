# $NetBSD: Makefile,v 1.19.4.1 2007/07/11 20:10:38 mjf Exp $

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
