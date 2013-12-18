#	$NetBSD: Makefile.inc,v 1.14.38.1 2013/12/18 18:43:29 matt Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

BFD_MACHINE_ARCH?=${MACHINE_ARCH:S/armv7/arm/}

GNUHOSTDIST=	${DIST}
GNUCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GNUCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GNUCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/${BFDSUBDIR}/libbfd/arch/${BFD_MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/bfd -I${DIST}/binutils \
		-DLOCALEDIR=\"${LOCALEDIR}\"

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
