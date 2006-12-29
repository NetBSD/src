# $NetBSD: Makefile,v 1.17 2006/12/29 17:00:08 pooka Exp $

.include <bsd.own.mk>

SUBDIR+=	adosfs
SUBDIR+=	cd9660
SUBDIR+=	coda
SUBDIR+=	coda5
SUBDIR+=	filecorefs
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
