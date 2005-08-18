#	$NetBSD: Makefile,v 1.6 2005/08/18 22:30:37 rpaulo Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c dns_rr_to_pa.c \
	dns_sa_to_rr.c dns_rr_eq_sa.c dns_rr_to_sa.c

.include <bsd.lib.mk>
