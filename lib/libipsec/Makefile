# $NetBSD: Makefile,v 1.5 2000/05/09 05:52:54 thorpej Exp $

LIB=	ipsec
CFLAGS+=-g
CPPFLAGS+=-DIPSEC_DEBUG -DIPSEC -DINET6 -I${.CURDIR} -I. -DYY_NO_UNPUT

SRCS+=	ipsec_strerror.c policy_parse.y policy_token.l
SRCS+=	ipsec_get_policylen.c ipsec_dump_policy.c
LPREFIX+=__libyy
YPREFIX+=__libyy
YHEADER=1

#.PATH:	${.CURDIR}/../../sys/netkey
#SRCS+=	pfkey.c pfkey_dump.c
#SRCS+=	key_debug.c

#LFLAGS+=	-olex.yy.c

MAN=	ipsec_set_policy.3 ipsec_strerror.3
MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

CLEANFILES+=	y.tab.h

.include <bsd.lib.mk>
