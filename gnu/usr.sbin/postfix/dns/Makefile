#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:44 lukem Exp $

NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
