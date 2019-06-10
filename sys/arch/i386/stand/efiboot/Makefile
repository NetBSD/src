#	$NetBSD: Makefile,v 1.1.22.1 2019/06/10 22:06:21 christos Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
