#	$Id: Makefile,v 1.4 1993/09/03 19:05:59 jtc Exp $

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
AINC+=	-I${LIBC}/${MACHINE}

.include <bsd.lib.mk>
