#	$NetBSD: Makefile.inc,v 1.9.12.1 2006/05/24 15:48:24 tron Exp $

RELOC=0x2f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I.
CPPFLAGS+=-I${.CURDIR}/../../../../
CPPFLAGS+=-I${.CURDIR}/../../
CPPFLAGS+=-I${.CURDIR}/../common
CPPFLAGS+=-I${.CURDIR}/../../include
CPPFLAGS+=-DRELOC=${RELOC}

WARNS?=	1
CFLAGS+=-ffreestanding
.if ${HAVE_GCC} == "2"
CFLAGS+=-mno-asm-pic
AFLAGS+=-mno-asm-pic
.endif

MKMAN=no

.if ${MACHINE} == "vax"
.PHONY: machine-links
beforedepend: machine-links
machine-links:
	@rm -f machine && ln -s ${S}/arch/${MACHINE}/include machine
	@rm -f ${MACHINE_ARCH} && ln -s ${S}/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}
.NOPATH: machine ${MACHINE_ARCH}
CLEANFILES+= machine ${MACHINE_ARCH}
.endif
