#       @(#)Makefile    6.2 (Berkeley) 3/25/91
PROG=           cc
SRCS=           gcc.c version.c obstack.c
MAN1=		gcc.0 g++.0
BINDIR=		/usr/bin
CFLAGS+=        -I$(.CURDIR) -I$(.CURDIR)/../common

LINKS+=	${DESTDIR}${BINDIR}/cc ${DESTDIR}${BINDIR}/gcc
LINKS+=	${DESTDIR}${BINDIR}/g++ ${DESTDIR}${BINDIR}/c++
MLINKS=	gcc.1 cc.1  g++.1 c++.1

.PATH:          $(.CURDIR)/../common 

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/g++.script $(DESTDIR)/usr/bin/g++

.include <bsd.prog.mk>
