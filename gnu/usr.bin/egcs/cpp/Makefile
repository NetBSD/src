#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.1.1.1 1998/03/29 11:34:24 mrg Exp $

.if	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "mips")
PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.c obstack.c version.c 
CPPFLAGS+= \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
YFLAGS=

CLEANFILES+= cexp.c
.endif

LINKS=	/usr/bin/cpp /usr/bin/gcpp
FILES=usr.bin.cpp.sh
FILESNAME=cpp
FILESDIR=/usr/bin
FILESMODE=${BINMODE}

MAN=	cpp.1
MLINKS=	cpp.1 cccp.1 cpp.1 gcpp.1

.include <bsd.prog.mk>
