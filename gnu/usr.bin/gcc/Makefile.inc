#	$NetBSD: Makefile.inc,v 1.6 2003/04/04 00:25:07 thorpej Exp $

.include <bsd.own.mk>

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/toolchain

GCCARCH=	${.CURDIR}/../arch/${MACHINE_ARCH}

CPPFLAGS+=	-DLOCALEDIR=\"/usr/share/locale\" -DNETBSD_NATIVE
HOST_CPPFLAGS+=	-I${.CURDIR}/..

.if ${USETOOLS} == "yes"
COMPATOBJDIR!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
NBCOMPATLIB=	-L${COMPATOBJDIR} -lnbcompat
.endif

.include "${GCCARCH}/defs.mk"
