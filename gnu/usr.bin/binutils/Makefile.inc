#	$NetBSD: Makefile.inc,v 1.11 2002/09/16 16:22:16 thorpej Exp $

.if !defined(__MAKEFILE_INC_INCLUDED__)
__MAKEFILE_INC_INCLUDED__=1

.include <bsd.own.mk>

GCPPFLAGS=	${G_DEFS} ${G_INCLUDES}
CPPFLAGS+=	${GCPPFLAGS:M-D*:N-DLOCALEDIR*} ${GCPPFLAGS:M-I*:N-I.*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH} \
		-DLOCALEDIR=\"${LOCALEDIR}\"

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # __MAKEFILE_INC_INCLUDED__
