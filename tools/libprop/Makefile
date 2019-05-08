#	$NetBSD: Makefile,v 1.2 2019/05/08 02:25:50 thorpej Exp $

HOSTLIB=	prop

.include <bsd.hostinit.mk>

LIBPROP_DIR=	${.CURDIR}/../../common/lib/libprop
LIBPROP_INC=	${.CURDIR}/../../common/include

.include "${LIBPROP_DIR}/Makefile.inc"

CPPFLAGS+=	-I${.CURDIR}/../compat -I${LIBPROP_INC}
CPPFLAGS+=	-I${TOOLDIR}/include/compat
CPPFLAGS+=	-I${TOOLDIR}/include/nbinclude

.PATH:		${LIBPROP_DIR}

CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1 -D_FILE_OFFSET_BITS=64

HOST_CPPFLAGS:=	${CPPFLAGS} ${HOST_CPPFLAGS}
CPPFLAGS:=	# empty

.include "${.CURDIR}/../Makefile.nbincludes"
.include <bsd.hostlib.mk>
