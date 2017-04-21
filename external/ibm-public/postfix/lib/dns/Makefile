#	$NetBSD: Makefile,v 1.1.36.1 2017/04/21 16:52:53 bouyer Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	dns_lookup.c dns_rr.c dns_strerror.c dns_strtype.c dns_rr_to_pa.c \
	dns_sa_to_rr.c dns_rr_eq_sa.c dns_rr_to_sa.c dns_rr_filter.c \
	dns_strrecord.c

.include <bsd.lib.mk>
