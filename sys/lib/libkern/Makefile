#	$Id: Makefile,v 1.6 1994/03/09 21:24:03 ws Exp $

LIB=	kern
NOPIC=
OBJMACHINE=
CFLAGS+=	-DKERNEL

.if exists (${.CURDIR}/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/${MACHINE_ARCH}
.include "${.CURDIR}/${MACHINE_ARCH}/Makefile.inc"
.endif
.PATH:	${.CURDIR}/kernel

LIBC=	${.CURDIR}/../../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE_ARCH}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE_ARCH}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE_ARCH}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE_ARCH}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE_ARCH}/sys
AINC+=	-I${LIBC}/arch/${MACHINE_ARCH}

# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c

# Disk subroutines
SRCS+=	disklib.c

.PATH: ${LIBC}/arch/${MACHINE_ARCH}/quad ${LIBC}/quad

install:

.include <bsd.lib.mk>
