#	$NetBSD: Makefile.inc,v 1.16.6.1 2016/03/19 11:30:06 skrll Exp $

NOSSP=
NOFORTIFY=
NOPIE=
NODEBUG=

.include <bsd.klinks.mk>

RELOC=0x3f0000
.PATH:	${.CURDIR}/../../vax ${.CURDIR}/../common

CPPFLAGS+=-I.
CPPFLAGS+=-I${.CURDIR}/../../../../
CPPFLAGS+=-I${.CURDIR}/../../
CPPFLAGS+=-I${.CURDIR}/../common
CPPFLAGS+=-I${.CURDIR}/../../include
CPPFLAGS+=-DRELOC=${RELOC}
CPPFLAGS+=-D_STANDALONE

WARNS?=	1
CFLAGS+=-ffreestanding -fno-pic -fno-unwind-tables
AFLAGS+=-fno-pic

MKMAN=no

LIBCRTBEGIN=
LIBCRTEND=  
