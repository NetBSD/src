#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:34:24 mrg Exp $

.if	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "mips")
PROG=	g++
SRCS=	gcc.c obstack.c version.c g++spec.c choose-temp.c pexecute.c
BINDIR=	/usr/bin
CPPFLAGS+= -DGCC_NAME=\"/usr/bin/cc\"

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++
.endif

MAN=	g++.1
MLINKS=	g++.1 c++.1

.include <bsd.prog.mk>

.PATH: $(DIST)/cp
CPPFLAGS+= -I$(DIST)/cp
