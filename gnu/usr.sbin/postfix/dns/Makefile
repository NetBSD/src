#	$NetBSD: Makefile,v 1.5 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c

.include <bsd.lib.mk>
