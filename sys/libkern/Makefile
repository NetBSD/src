#	$Id: Makefile,v 1.6 1993/09/10 23:32:17 jtc Exp $

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

.include <bsd.lib.mk>
