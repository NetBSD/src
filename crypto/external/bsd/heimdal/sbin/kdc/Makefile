# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:17 mrg Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network server

.PATH: ${HEIMDIST}/kdc

PROG= kdc

# XXXrcd: ??? should I include this?
#	announce.c		\

SRCS= connect.c config.c main.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMDIST}/lib

LDADD+=	-lkdc -lhdb -lheimntlm -lutil
DPADD+=	${LIBKDC} ${LIBHDB} ${LIBHEIMNTLM} ${LIBUTIL}

MAN=	kdc.8

# XXX: Pull this in from hcrypto.  Be sure to be last!
SRCS+=	ui.c
.PATH:	${HEIMDIST}/lib/hcrypto
CPPFLAGS.ui.c+= 		\
	-I${HEIMDIST}/lib/hcrypto

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
