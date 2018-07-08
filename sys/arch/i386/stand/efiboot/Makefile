#	$NetBSD: Makefile,v 1.3 2018/07/08 17:01:02 christos Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
