# $NetBSD: Makefile,v 1.2 1999/07/03 06:59:28 itojun Exp $

LIB=	ipsec
#CFLAGS+=-g
CPPFLAGS+=-DIPSEC_DEBUG
CPPFLAGS+=-DIPSEC
CPPFLAGS+=-DINET6

.PATH:	${.CURDIR}/../../sys/netkey
SRCS=	pfkey.c pfkey_dump.c
SRCS+=	ipsec_policy.c ipsec_strerror.c key_debug.c

MAN=	ipsec_set_policy.3 ipsec_strerror.3
MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
	ipsec_set_policy.3 ipsec_dump_policy.3

.include <bsd.lib.mk>
