#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.4 1997/03/24 22:06:19 christos Exp $

PROG=		cpp
BINDIR=		/usr/libexec
SRCS=	cccp.c obstack.c cexp.c version.c 
CFLAGS+=	-I. -I$(.CURDIR) -I$(.CURDIR)/../common -I$(.CURDIR)/../arch \
                -I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
		-DGCC_INCLUDE_DIR=\"\" \
		-DTOOL_INCLUDE_DIR=\"\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}
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
