#	$NetBSD: Makefile,v 1.2 1997/03/14 04:59:34 thorpej Exp $

S=	${.CURDIR}/../../../../

PROG= biosboot
NOMAN=

SRCS= main.c devopen.c conf.c version.c

CLEANFILES+= ${BSSTART} ${PROG}.sym ${PROG}.list

CPPFLAGS+= -DCOMPAT_OLDBOOT
#should go into SAMISCCPPFLAGS after system upgrade
CPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
#CPPFLAGS+= -DMAXFLENTRIES=10

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
#needs some cleanup in libsa
#CFLAGS+= -Wall

# XXX should go into library
SRCS+= ls.c
.PATH: ${.CURDIR}/../libsa

${PROG}: ${PROG}.sym
	@# do nothing

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG}.sym ${DESTDIR}${BINDIR}


SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no

.include "../Makefile.booters"
