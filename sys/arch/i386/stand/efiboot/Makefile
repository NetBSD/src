#	$NetBSD: Makefile,v 1.1.20.2 2018/11/26 01:52:25 pgoyette Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
