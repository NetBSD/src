#	$NetBSD: Makefile,v 1.22 1995/09/26 18:27:24 jonathan Exp $

LIB=	kern
NOPIC=

DIR=${KERNREL}${KERNDIR}

.if exists (${DIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${DIR}/arch/${MACHINE_ARCH}
.include "${DIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

.PATH: ${DIR}

CC = ${KERNCC}
CFLAGS += -I${DIR}/arch/${MACHINE_ARCH} \
	${KERNCFLAGS:S/-D_KERNEL//:S@-I.@-I${KERNREL}.@g}

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	mcount.c

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

install:

.include <bsd.lib.mk>
