#	$NetBSD: Makefile,v 1.5 2002/09/19 03:09:46 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
