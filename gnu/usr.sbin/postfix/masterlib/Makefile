#	$NetBSD: Makefile,v 1.2 2001/03/13 18:13:20 itojun Exp $

LIB=	master

DIST=	${.CURDIR}/../../../dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
