#	$Id: Makefile,v 1.1 1993/10/13 05:40:34 cgd Exp $

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
.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE}/sys
AINC+=	-I${LIBC}/arch/${MACHINE}

install:
	install ${COPY} lib${LIB}.a ${LIBDIR}
	${RANLIB} -t ${LIBDIR}/lib${LIB}.a
.if !defined(NOPROFILE)
	install ${COPY} lib${LIB}_p.a ${LIBDIR}
	${RANLIB} -t ${LIBDIR}/lib${LIB}_p.a
.endif

.include <bsd.lib.mk>
