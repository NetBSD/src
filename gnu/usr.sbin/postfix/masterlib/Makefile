#	$NetBSD: Makefile,v 1.4 2002/02/02 23:21:47 perry Exp $

LIB=	master

DIST=	${.CURDIR}/../../../dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

# only needed during build
libinstall::

.include <bsd.lib.mk>
