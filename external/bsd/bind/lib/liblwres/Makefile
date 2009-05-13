#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:51:50 jym Exp $

LIB=lwres

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/lwres
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS =	context.c gai_strerror.c getaddrinfo.c gethost.c getipnode.c \
	getnameinfo.c getrrset.c herror.c lwbuffer.c lwconfig.c lwpacket.c \
	lwresutil.c lwres_gabn.c lwres_gnba.c lwres_grbn.c lwres_noop.c \
	lwinetaton.c lwinetpton.c lwinetntop.c

.include <bsd.lib.mk>
