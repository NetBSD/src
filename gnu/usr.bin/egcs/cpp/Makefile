#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.5 1998/04/02 06:45:53 mrg Exp $

PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.y obstack.c version.c 
CPPFLAGS+= \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"

LINKS=	/usr/bin/cpp /usr/bin/gcpp
FILES=usr.bin.cpp.sh
FILESNAME=cpp
FILESDIR=/usr/bin
FILESMODE=${BINMODE}

MAN=	cccp.1
MLINKS=	cccp.1 cpp.1 cccp.1 gcpp.1

.PATH: $(.OBJDIR)

.include <bsd.prog.mk>
