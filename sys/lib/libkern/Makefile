#	$NetBSD: Makefile,v 1.19 1995/02/13 21:48:17 cgd Exp $

LIB=	kern
NOPIC=
OBJMACHINE=
CFLAGS= -O3 -I${.CURDIR} -Imachine/.. -I. -I${.CURDIR}/../..

machine-links:
	-@rm -f machine && \
	    ln -s ${.CURDIR}/../../arch/${MACHINE}/include machine
	-@rm -f ${MACHINE_ARCH} && \
	    ln -s ${.CURDIR}/../../arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}

lib${LIB}.a lib${LIB}_p.a::	machine-links

.if exists (${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc)
.PATH:	${.CURDIR}/arch/${MACHINE_ARCH}
.include "${.CURDIR}/arch/${MACHINE_ARCH}/Makefile.inc"
.endif

LIBC=	${.CURDIR}/../../../lib/libc
.PATH: ${LIBC}/string ${LIBC}/arch/${MACHINE_ARCH}/string
.PATH: ${LIBC}/net ${LIBC}/arch/${MACHINE_ARCH}/net
.PATH: ${LIBC}/gen ${LIBC}/arch/${MACHINE_ARCH}/gen
.PATH: ${LIBC}/stdlib ${LIBC}/arch/${MACHINE_ARCH}/stdlib
.PATH: ${LIBC}/sys ${LIBC}/arch/${MACHINE_ARCH}/sys
AINC+=	-I${LIBC}/arch/${MACHINE_ARCH}
CFLAGS+= -I${.CURDIR}/arch/${MACHINE_ARCH}

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

CLEANFILES+= machine ${MACHINE_ARCH}

install:

.include <bsd.lib.mk>
