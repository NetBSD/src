#	$NetBSD: Makefile,v 1.4 2022/10/08 16:12:51 christos Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	dns

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS= \
dns_lookup.c \
dns_rr.c \
dns_rr_eq_sa.c \
dns_rr_filter.c \
dns_rr_to_pa.c \
dns_rr_to_sa.c \
dns_sa_to_rr.c \
dns_sec.c \
dns_str_resflags.c \
dns_strerror.c \
dns_strrecord.c \
dns_strtype.c \
test_dns_lookup.c

.include <bsd.lib.mk>
