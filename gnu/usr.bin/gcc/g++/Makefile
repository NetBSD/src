#	from: @(#)Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.6 1997/06/23 13:25:48 mrg Exp $

.if	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax") || \
	(${MACHINE_ARCH} == "arm32") || \
	(${MACHINE_ARCH} == "mips") 
PROG=	g++
SRCS=	g++.c obstack.c version.c 
BINDIR=	/usr/bin
CFLAGS+=-I$(.CURDIR) -I$(.CURDIR)/../common \
	-I$(.CURDIR)/../arch -I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
	-DGCC_NAME=\"/usr/bin/cc\"
LDADD+=	-lgnumalloc
DPADD+=	${LIBGNUMALLOC}

LINKS+=	${BINDIR}/g++ ${BINDIR}/c++

.PATH:          $(.CURDIR)/../common 
.endif

MAN=	g++.1
MLINKS=	g++.1 c++.1

.include <bsd.prog.mk>
