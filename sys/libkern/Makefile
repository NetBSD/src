#	$Id: Makefile,v 1.9 1993/10/07 19:40:36 cgd Exp $

LIB=	kern
LIBDIR=	${.CURDIR}/../arch/${MACHINE}/compile
NOPIC=

.if exists (${.CURDIR}/${MACHINE}/Makefile.inc)
.PATH:	${.CURDIR}/${MACHINE}
.include "${.CURDIR}/${MACHINE}/Makefile.inc"
.endif

.if defined(ARCH) && !empty(ARCH)
.if exists (${.CURDIR}/${ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/${ARCH}
.include "${.CURDIR}/${ARCH}/Makefile.inc"
.endif
.endif

LIBC=	${.CURDIR}/../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/${MACHINE}/string
.PATH: ${LIBC}/net ${LIBC}/${MACHINE}/net
.PATH: ${LIBC}/gen ${LIBC}/${MACHINE}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/${MACHINE}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/${MACHINE}/sys
AINC+=	-I${LIBC}/${MACHINE}

install:
	install ${COPY} lib${LIB}.a ${LIBDIR}
	${RANLIB} -t ${LIBDIR}/lib${LIB}.a
.if !defined(NOPROFILE)
	install ${COPY} lib${LIB}_p.a ${LIBDIR}
	${RANLIB} -t ${LIBDIR}/lib${LIB}_p.a
.endif

.include <bsd.lib.mk>
