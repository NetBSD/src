#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.2 1998/03/30 04:15:36 mrg Exp $

PROG=	g++
SRCS=	gcc.c obstack.c version.c g++spec.c choose-temp.c pexecute.c
BINDIR=	/usr/bin
CPPFLAGS+= -DGCC_NAME=\"/usr/bin/cc\"

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++

MAN=	g++.1
MLINKS=	g++.1 c++.1

.include <bsd.prog.mk>

.PATH: $(DIST)/cp
CPPFLAGS+= -I$(DIST)/cp
