#	$NetBSD: Makefile.inc,v 1.17.16.1 2018/03/30 06:20:12 pgoyette Exp $

NOSSP=
NOFORTIFY=
NOPIE=
NODEBUG=

.include <bsd.klinks.mk>

RELOC=0x7f0000
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
