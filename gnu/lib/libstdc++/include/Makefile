#	$NetBSD: Makefile,v 1.2 2002/08/19 09:46:42 lukem Exp $

.include <bsd.own.mk>

# This include file keeps the includes/build of libstdc++ separate,
# to avoid issues like "iostream.cc" using the default ".cc:" rule
# to build the "iostream" file (which is actually a header).

.if defined(USE_NEW_TOOLCHAIN)

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

INCS=		${G_HEADERS} ${STD_HEADERS} ${STL_HEADERS}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++ ${DIST}/libstdc++/stl

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

.endif
