#	$Id: Makefile,v 1.5 1993/09/05 21:39:52 deraadt Exp $

LIB=	kern
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
