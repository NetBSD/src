# $NetBSD: Makefile,v 1.5 1999/05/12 12:05:56 tron Exp $

.if (${MACHINE} == "amiga")
SUBDIR+=	adosfs
.endif
.if (${MACHINE} == "alpha") ||	\
    (${MACHINE} == "i386") ||	\
    (${MACHINE} == "ofppc")
SUBDIR+=	ntfs
.endif

SUBDIR+=	isofs miscfs msdosfs ufs coda

.include <bsd.subdir.mk>
