#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:57 perry Exp $

LIB=	dns

DIST=	${.CURDIR}/../../../dist/postfix/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
