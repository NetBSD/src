#	$NetBSD: Makefile,v 1.8 2002/08/19 09:46:42 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

# This include file keeps the includes/build of libstdc++ separate,
# to avoid issues like "iostream.cc" using the default ".cc:" rule
# to build the "iostream" file (which is actually a header).

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

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

.PATH: ${NETBSDSRCDIR}/gnu/dist/libio

.include <bsd.prog.mk>

.endif
