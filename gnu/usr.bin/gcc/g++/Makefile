#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.9 1998/07/28 18:53:41 thorpej Exp $

.if	(${MACHINE_ARCH} != "alpha") && \
	(${MACHINE_ARCH} != "powerpc")
PROG=	g++
SRCS=	g++.c obstack.c version.c 
BINDIR=	/usr/bin
CPPFLAGS+=-I$(.CURDIR) -I$(.CURDIR)/../common \
	-I$(.CURDIR)/../arch -I$(.CURDIR)/../arch/$(MACHINE_ARCH:C/^mips.*/mips/) \
	-DGCC_NAME=\"/usr/bin/cc\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++

.PATH:          $(.CURDIR)/../common 
.endif

MAN=	g++.1
MLINKS=	g++.1 c++.1

.include <bsd.prog.mk>
