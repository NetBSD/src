#	$NetBSD: Makefile,v 1.10 1999/01/30 17:44:10 christos Exp $

S=	${.CURDIR}/../../../../

BASE= dosboot
PROG= ${BASE}.com
NOMAN=
NEWVERSWHAT=	"DOS Boot"

SRCS= main.c devopen.c exec.c

CLEANFILES+= ${DOSSTART} ${BASE}.sym

CPPFLAGS+= -DCOMPAT_OLDBOOT -DXMS
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall

# XXX should go into library
SRCS+= getopt.c ls.c
.PATH: ${.CURDIR}/../libsa

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no
I386MISCMAKEFLAGS= I386_INCLUDE_DOS=yes

.include "../Makefile.booters"
