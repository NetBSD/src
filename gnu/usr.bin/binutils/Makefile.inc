#	$NetBSD: Makefile.inc,v 1.12 2003/03/01 13:09:01 mrg Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist/toolchain

GCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/bfd -I${DIST}/binutils \
		-DLOCALEDIR=\"${LOCALEDIR}\"

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
