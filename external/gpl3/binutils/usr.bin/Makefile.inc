#	$NetBSD: Makefile.inc,v 1.5 2013/08/05 00:08:58 matt Exp $

BINDIR?=	/usr/bin

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

BINUTILS_MACHINE_ARCH=	${MACHINE_ARCH:C/armv[467]/arm/}

GNUHOSTDIST=	${DIST}
GNUCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GNUCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GNUCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/${BFDSUBDIR}/libbfd/arch/${BINUTILS_MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/bfd -I${DIST}/binutils \
		-DLOCALEDIR=\"${LOCALEDIR}\"
NOGCCERROR=	# defined

.endif # __MAKEFILE_INC_INCLUDED__
