#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.5 1997/05/13 21:02:00 phil Exp $

PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.c obstack.c version.c 
CFLAGS+=-I. -I$(.CURDIR) -I$(.CURDIR)/../common -I$(.CURDIR)/../arch \
	-I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}
YFLAGS=

MLINKS=	cpp.1 cccp.1 cpp.1 gcpp.1

CLEANFILES+= cexp.c

.PATH:		$(.CURDIR)/../common

LINKS=	/usr/bin/cpp /usr/bin/gcpp
FILES=usr.bin.cpp.sh
FILESNAME=cpp
FILESDIR=/usr/bin
FILESMODE=${BINMODE}

.include <bsd.prog.mk>
