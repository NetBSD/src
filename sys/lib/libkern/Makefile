#	$Id: Makefile,v 1.4 1993/12/04 03:04:30 cgd Exp $

LIB=	kern
NOPIC=
OBJMACHINE=

.if exists (${.CURDIR}/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/${MACHINE_ARCH}
.include "${.CURDIR}/${MACHINE_ARCH}/Makefile.inc"
.endif

LIBC=	${.CURDIR}/../../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE_ARCH}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE_ARCH}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE_ARCH}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE_ARCH}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE_ARCH}/sys
AINC+=	-I${LIBC}/arch/${MACHINE_ARCH}

install:

.include <bsd.lib.mk>
