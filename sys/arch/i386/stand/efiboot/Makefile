#	$NetBSD: Makefile,v 1.5 2018/11/08 23:20:52 christos Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
