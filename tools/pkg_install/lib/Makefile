# $NetBSD: Makefile,v 1.1 2020/01/12 21:49:39 christos Exp $
.include <bsd.own.mk>

HOSTLIB=	${LIB}

HOST_CPPFLAGS+=	${CPPFLAGS}
HOST_SRCS+=	${SRCS}

.include "${NETBSDSRCDIR}/external/bsd/pkg_install/lib/Makefile"
.include "${.CURDIR}/../Makefile.inc"
