#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.3 1998/03/30 04:15:36 mrg Exp $

PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.c obstack.c version.c 
CPPFLAGS+= \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
YFLAGS=

CLEANFILES+= cexp.c

LINKS=	/usr/bin/cpp /usr/bin/gcpp
FILES=usr.bin.cpp.sh
FILESNAME=cpp
FILESDIR=/usr/bin
FILESMODE=${BINMODE}

MAN=	cccp.1
MLINKS=	cccp.1 cpp.1 cccp.1 gcpp.1

.include <bsd.prog.mk>
