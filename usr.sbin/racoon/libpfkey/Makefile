#	$NetBSD: Makefile,v 1.3 2002/09/18 03:54:36 lukem Exp $

NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

.include <bsd.own.mk>

LIB=	pfkey
CFLAGS+=-g -Wall -Werror
CPPFLAGS+=-DIPSEC_DEBUG -DIPSEC -DINET6 -I. -DYY_NO_UNPUT -I${LIBPFKEYSRCDIR}

SRCS=	pfkey.c pfkey_dump.c ipsec_strerror.c
SRCS+=	key_debug.c
#YPREFIX=__libyy
#LPREFIX=__libyy
#YHEADER=1

#MAN=	ipsec_set_policy.3 ipsec_strerror.3
#MLINKS+=ipsec_set_policy.3 ipsec_get_policylen.3 \
#	ipsec_set_policy.3 ipsec_dump_policy.3

# don't install it into public place!
libinstall::

.include <bsd.lib.mk>

.PATH:	${LIBPFKEYSRCDIR} ${NETBSDSRCDIR}/sys/netkey
