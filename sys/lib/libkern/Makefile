#	$Id: Makefile,v 1.3 1993/10/23 02:33:18 cgd Exp $

LIB=	kern
NOPIC=
OBJMACHINE=

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

LIBC=	${.CURDIR}/../../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE}/sys
AINC+=	-I${LIBC}/arch/${MACHINE}

install:

.include <bsd.lib.mk>
