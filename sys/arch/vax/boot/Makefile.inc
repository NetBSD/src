#	$NetBSD: Makefile.inc,v 1.5.8.1 2002/02/28 04:12:26 nathanw Exp $

RELOC=0x2f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I. -I${.CURDIR}/../../../../ -I${.CURDIR}/../../ -I${.CURDIR}/../common -I${.CURDIR}/../../include
CPPFLAGS+=-DRELOC=${RELOC}
