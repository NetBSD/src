#	$Id: Makefile,v 1.7 1993/09/13 19:53:18 jtc Exp $

LIB=	kern
LIBDIR=	${.CURDIR}/../arch/${MACHINE}/compile

NOPROFILE=

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

.include <bsd.lib.mk>
