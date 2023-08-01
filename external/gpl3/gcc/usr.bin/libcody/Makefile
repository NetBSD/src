#	$NetBSD: Makefile,v 1.1 2023/08/01 05:57:55 mrg Exp $

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

HOSTPROG_CXX=	1
LIBISPRIVATE=   yes

.include <bsd.init.mk>

LIB=		cody

SRCS=		buffer.cc client.cc fatal.cc netclient.cc netserver.cc packet.cc resolver.cc server.cc

CPPFLAGS+=	-I${DIST}/libgcc \
		-DSRCDIR=\"\"

.include <bsd.lib.mk>

.PATH: ${DIST}/libcody
