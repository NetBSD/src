#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.11 1998/12/09 14:44:03 christos Exp $

.if	(${MACHINE_ARCH} != "alpha") && \
	(${MACHINE_ARCH} != "powerpc")
PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.y obstack.c version.c 
CPPFLAGS+=-I. -I$(.CURDIR) -I$(.CURDIR)/../common -I$(.CURDIR)/../arch \
	-I$(.CURDIR)/../arch/$(MACHINE_ARCH:C/^mips.*/mips/) \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}
CPPFLAGS+=-DLINTCOMMENT

.PATH:		$(.CURDIR)/../common
.endif

LINKS=	/usr/bin/cpp /usr/bin/gcpp
FILES=usr.bin.cpp.sh
FILESNAME=cpp
FILESDIR=/usr/bin
FILESMODE=${BINMODE}

MAN=	cpp.1
MLINKS=	cpp.1 cccp.1 cpp.1 gcpp.1

.include <bsd.prog.mk>
