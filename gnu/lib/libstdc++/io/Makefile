#	$NetBSD: Makefile,v 1.4 2001/08/10 11:57:49 mrg Exp $

.include <bsd.own.mk>

# This include file keeps the includes/build of libstdc++ separate,
# to avoid issues like "iostream.cc" using the default ".cc:" rule
# to build the "iostream" file (which is actually a header).

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

INCS=		${LIBIO_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl ${DIST}/libio

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.else

INCS=	PlotFile.h SFile.h builtinbuf.h editbuf.h floatio.h fstream.h \
	indstream.h iolibio.h iomanip.h iostdio.h iostream.h iostreamP.h \
	istream.h libio.h libioP.h ostream.h parsestream.h pfstream.h \
	procbuf.h stdiostream.h stream.h streambuf.h strfile.h strstream.h
INCSDIR=/usr/include/g++

MKOBJ=	no

.PATH: ${.CURDIR}/../../../dist/libio

.include <bsd.prog.mk>

.endif
