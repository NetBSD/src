#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.7 1997/10/25 10:11:04 lukem Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "mips") 
PROG=	cpp
BINDIR=	/usr/libexec
SRCS=	cccp.c cexp.c obstack.c version.c 
CPPFLAGS+=-I. -I$(.CURDIR) -I$(.CURDIR)/../common -I$(.CURDIR)/../arch \
	-I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
	-DGCC_INCLUDE_DIR=\"\" \
	-DTOOL_INCLUDE_DIR=\"\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}
YFLAGS=

CLEANFILES+= cexp.c

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
