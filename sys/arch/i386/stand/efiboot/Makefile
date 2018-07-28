#	$NetBSD: Makefile,v 1.1.20.1 2018/07/28 04:37:35 pgoyette Exp $

SUBDIR=	bootia32
.if ${MACHINE} == "amd64"
SUBDIR+=	bootx64
.endif

.include <bsd.subdir.mk>
