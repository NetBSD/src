# $NetBSD: Makefile,v 1.10 2005/02/19 16:55:04 thorpej Exp $

USE_SHLIBDIR=	yes
NOLINT=		# Will be fixed later
WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
LIB=	ipsec
CFLAGS+=-g
CPPFLAGS+=	-I${DIST}/src/libipsec -I. -DHAVE_CONFIG_H \
		-DIPSEC_DEBUG -I${.CURDIR} -I${NETBSDSRCDIR}/sys

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH:	${DIST}/src/libipsec

SRCS=	ipsec_dump_policy.c ipsec_get_policylen.c ipsec_strerror.c	\
	pfkey.c pfkey_dump.c policy_parse.y policy_token.l key_debug.c
LPREFIX+=__libipsec
YPREFIX+=__libipsec
YHEADER=1

MAN=	ipsec_set_policy.3 ipsec_strerror.3
MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

CLEANFILES+=	ipsec_set_policy.cat3 ipsec_strerror.cat3

.include <bsd.lib.mk>
