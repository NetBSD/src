#	$NetBSD: Makefile,v 1.2 2018/07/08 16:50:28 christos Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR=		bootx64
.endif

.include <bsd.subdir.mk>
