#	$NetBSD: Makefile,v 1.6.2.1 1997/08/23 07:09:25 thorpej Exp $

S=	${.CURDIR}/../../../../

PROG= dosboot
NOMAN=
NEWVERSWHAT=	"DOS Boot"

SRCS= main.c devopen.c

CLEANFILES+= ${DOSSTART} ${PROG}.sym ${PROG}.com ${PROG}.list

CPPFLAGS+= -DCOMPAT_OLDBOOT -DXMS
#uncomment if there are problems with memory detection
#CPPFLAGS+= -DCONSERVATIVE_MEMDETECT

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall

# XXX should go into library
SRCS+= getopt.c ls.c
.PATH: ${.CURDIR}/../libsa

${PROG}: ${PROG}.com
	@# do nothing

proginstall::
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG}.com ${DESTDIR}${BINDIR}

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no
I386MISCMAKEFLAGS= I386_INCLUDE_DOS=yes

.include "../Makefile.booters"
