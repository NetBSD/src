#	$NetBSD: Makefile.inc,v 1.5 2003/03/14 03:44:11 thorpej Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat

.include "${GCCARCH}/defs.mk"
