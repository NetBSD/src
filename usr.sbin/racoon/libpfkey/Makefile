#	$NetBSD: Makefile,v 1.6 2005/01/10 02:58:59 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	pfkey
#CFLAGS+=-g -Wall -Werror
CPPFLAGS+=-DIPSEC_DEBUG -DIPSEC -I. -DYY_NO_UNPUT -I${LIBPFKEYSRCDIR}

SRCS=	pfkey.c pfkey_dump.c ipsec_strerror.c
SRCS+=	key_debug.c
#YPREFIX=__libyy
#LPREFIX=__libyy
#YHEADER=1

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

#MAN=	ipsec_set_policy.3 ipsec_strerror.3
#MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
#	ipsec_set_policy.3 ipsec_dump_policy.3

.include <bsd.lib.mk>

.PATH:	${LIBPFKEYSRCDIR} ${NETBSDSRCDIR}/sys/netkey
