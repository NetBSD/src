#	$NetBSD: Makefile.inc,v 1.1 2009/08/18 20:22:07 skrll Exp $

BINDIR?=	/usr/bin

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

GNUHOSTDIST=	${DIST}
GNUCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GNUCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GNUCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/${BFDSUBDIR}/libbfd/arch/${MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/bfd -I${DIST}/binutils \
		-DLOCALEDIR=\"${LOCALEDIR}\"

.endif # __MAKEFILE_INC_INCLUDED__
