#	$NetBSD: Makefile.inc,v 1.19 2023/01/29 17:00:12 tsutsui Exp $

NOSSP=
NOFORTIFY=
NOPIE=
NODEBUG=

.include <bsd.klinks.mk>

RELOC=0x7d0000
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
