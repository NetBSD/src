# $NetBSD: Makefile,v 1.15 2005/12/15 04:27:32 jmc Exp $

USE_SHLIBDIR=	yes
WARNS=	0	# Will be fixed later

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/crypto/dist/ipsec-tools
LIB=	ipsec
CPPFLAGS+= -I${DIST}/src/libipsec -I. -DHAVE_CONFIG_H
CPPFLAGS+= -DIPSEC_DEBUG -I${.CURDIR} -I${NETBSDSRCDIR}/sys
CPPFLAGS+= -DSADB_X_EALG_AESCBC=SADB_X_EALG_AES
# Don't worry about argument promotion for now.
LINTFLAGS+=     -X 58

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH:	${DIST}/src/libipsec

SRCS=	ipsec_dump_policy.c ipsec_get_policylen.c ipsec_strerror.c	\
	pfkey.c pfkey_dump.c policy_parse.c policy_token.c key_debug.c

MAN=	ipsec_set_policy.3 ipsec_strerror.3
MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

.include <bsd.lib.mk>

# Don't auto-frob .y or .l files.
.l.c .y.c .y.h:
	@true
