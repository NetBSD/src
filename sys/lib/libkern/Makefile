#	$NetBSD: Makefile,v 1.21 1995/06/26 03:25:46 christos Exp $

LIB=	kern
NOPIC=

DIR=${KERNREL}${KERNDIR}

.if exists (${DIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${DIR}/arch/${MACHINE_ARCH}
.include "${DIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

LIBC=	${DIR}/../../../lib/libc

.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE_ARCH}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE_ARCH}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE_ARCH}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE_ARCH}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE_ARCH}/sys
.PATH: ${DIR}

AINC+=	-I${LIBC}/arch/${MACHINE_ARCH}
CC = ${KERNCC}
CFLAGS += -I${DIR}/arch/${MACHINE_ARCH} \
	${KERNCFLAGS:S/-D_KERNEL//:S@-I.@-I${KERNREL}.@g}

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.PATH: ${LIBC}/arch/${MACHINE_ARCH}/quad ${LIBC}/quad
.endif

# Other stuff
SRCS+=	mcount.c

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

install:

.include <bsd.lib.mk>
