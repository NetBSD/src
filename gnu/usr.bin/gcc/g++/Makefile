#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.1.1.1 1995/12/01 17:59:58 phil Exp $

PROG=           g++
SRCS=	g++.c obstack.c version.c 
MAN=		g++.1
BINDIR=		/usr/bin
CFLAGS+=        -I$(.CURDIR) -I$(.CURDIR)/../common \
                 -I$(.CURDIR)/../arch -I$(.CURDIR)/../arch/$(MACHINE_ARCH)
LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a

LINKS+=	${BINDIR}/c++ ${BINDIR}/g++
MLINKS=	g++.1 c++.1

.PATH:          $(.CURDIR)/../common 

.include <bsd.prog.mk>
