#	$NetBSD: Makefile,v 1.2 2001/03/13 18:13:18 itojun Exp $

LIB=	dns

DIST=	${.CURDIR}/../../../dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c

MKLINT=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build
libinstall::

.include <bsd.lib.mk>
