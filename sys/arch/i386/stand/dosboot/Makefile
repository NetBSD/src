#	$NetBSD: Makefile,v 1.5 1997/06/13 17:49:23 drochner Exp $

S=	${.CURDIR}/../../../../

PROG= dosboot
NOMAN=

SRCS= main.c devopen.c version.c

CLEANFILES+= ${DOSSTART} ${PROG}.sym ${PROG}.com ${PROG}.list

CPPFLAGS+= -DCOMPAT_OLDBOOT -DXMS

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
