#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.5 1997/05/13 21:02:01 phil Exp $

PROG=	g++
SRCS=	g++.c obstack.c version.c 
MAN=	g++.1
BINDIR=	/usr/bin
CFLAGS+=-I$(.CURDIR) -I$(.CURDIR)/../common \
	-I$(.CURDIR)/../arch -I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
	-DGCC_NAME=\"/usr/bin/cc\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++
MLINKS=	g++.1 c++.1

.PATH:          $(.CURDIR)/../common 

.include <bsd.prog.mk>
