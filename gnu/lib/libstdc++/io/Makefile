#	$NetBSD: Makefile,v 1.7.2.1 2004/03/28 07:47:43 jmc Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

INCS=		${LIBIO_HEADERS:Niostdio.h}
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

.PATH: ${.CURDIR}/../../../dist/libio

.include <bsd.prog.mk>

.endif
