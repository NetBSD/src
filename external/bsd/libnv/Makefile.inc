# $NetBSD: Makefile.inc,v 1.1.4.2 2019/06/10 21:44:58 christos Exp $

.include <bsd.own.mk>

LIBNV_DIR=	${NETBSDSRCDIR}/sys/external/bsd/libnv
LIBNV_DIST=	${LIBNV_DIR}/dist

#CPPFLAGS+=	-DWITH_MSGIO
CPPFLAGS+=	-D_VA_LIST_DEFINED -D__FBSDID\(x\)=
CPPFLAGS+=	-I${LIBNV_DIST}

.PATH:		${LIBNV_DIST}
