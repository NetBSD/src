#	$NetBSD: Makefile,v 1.5 1997/06/19 11:46:44 drochner Exp $

S=	${.CURDIR}/../../../../

PROG= biosboot
NOMAN=

SRCS= main.c devopen.c conf.c version.c

CLEANFILES+= ${BSSTART} ${PROG}.sym ${PROG}.list

CPPFLAGS+= -DCOMPAT_OLDBOOT
#increase MAXFLENTRIES if "installboot" complains about
# "not enough fragment space in bootcode" (default: 5)
#CPPFLAGS+= -DMAXFLENTRIES=10
#set PRIM_LOADSZ to <=9 to make a 720k boot floppy (default: 15)
#CPPFLAGS+= -DPRIM_LOADSZ=9

#CFLAGS= -O2 -fomit-frame-pointer -fno-defer-pop
CFLAGS+= -Wall

# XXX should go into library
SRCS+= ls.c
.PATH: ${.CURDIR}/../libsa

${PROG}: ${PROG}.sym
	@# do nothing

proginstall::
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${PROG}.sym ${DESTDIR}${BINDIR}

SAMISCCPPFLAGS+= -DHEAP_START=0x10000 -DHEAP_LIMIT=0x40000
SAMISCMAKEFLAGS= SA_USE_CREAD=yes SA_INCLUDE_NET=no

.include "../Makefile.booters"
