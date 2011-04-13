# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:59 elric Exp $

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

LDADD+=	-lkdc ${LIBVERS}
#LDADD+=	-lkdc -lhdb -lkrb5 -lhx509 -lheimntlm -lcrypto -lasn1 -lcom_err \
#	-lroken -lcrypt -lutil ${LIBVERS}
DPADD+=	${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${LIBVERS} ${LIBROKEN} \
	${LIBCRYPT} ${LIBUTIL}

MAN=	kdc.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
