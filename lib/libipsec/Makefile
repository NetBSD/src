# $NetBSD: Makefile,v 1.13 2005/02/24 13:45:08 manu Exp $

USE_SHLIBDIR=	yes
WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
LIB=	ipsec
CPPFLAGS+= -I${DIST}/src/libipsec -I. -DHAVE_CONFIG_H
CPPFLAGS+= -DIPSEC_DEBUG -I${.CURDIR} -I${NETBSDSRCDIR}/sys
CPPFLAGS+= -DSADB_X_EALG_AESCBC=SADB_X_EALG_AES

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH:	${DIST}/src/libipsec

SRCS=	ipsec_dump_policy.c ipsec_get_policylen.c ipsec_strerror.c	\
	pfkey.c pfkey_dump.c policy_parse.y policy_token.l key_debug.c

LPREFIX+=__libipsec
YPREFIX+=__libipsec
YHEADER=policy_parse.h

MAN=	ipsec_set_policy.3 ipsec_strerror.3
MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

.include <bsd.lib.mk>
