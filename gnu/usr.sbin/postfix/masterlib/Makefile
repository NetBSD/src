#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:08 tv Exp $

LIB=	master

DIST=	${.CURDIR}/../../../dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
