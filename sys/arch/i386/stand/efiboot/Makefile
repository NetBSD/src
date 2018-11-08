#	$NetBSD: Makefile,v 1.4 2018/11/08 21:47:04 christos Exp $

.include <bsd.init.mk>

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
