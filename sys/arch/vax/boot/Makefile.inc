#	$NetBSD: Makefile.inc,v 1.7 2003/12/08 07:32:19 matt Exp $

RELOC=0x2f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I. -I${.CURDIR}/../../../../ -I${.CURDIR}/../../ -I${.CURDIR}/../common -I${.CURDIR}/../../include
CPPFLAGS+=-DRELOC=${RELOC}

CFLAGS+=-ffreestanding
.if ${HAVE_GCC3} == "no"
CFLAGS+=-mno-asm-pic
AFLAGS+=-mno-asm-pic
.endif
