#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:03 perry Exp $

LIB=	master

DIST=	${.CURDIR}/../../../dist/postfix/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
