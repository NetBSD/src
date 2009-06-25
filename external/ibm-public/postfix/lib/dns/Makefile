#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:52 tron Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c dns_rr_to_pa.c \
	dns_sa_to_rr.c dns_rr_eq_sa.c dns_rr_to_sa.c

.include <bsd.lib.mk>
