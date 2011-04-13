# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:57 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kadmin

PROG=	kadmind
MAN=	kadmind.8

SRCS+= 	kadmind.c	\
	rpc.c		\
	server.c	\
	kadm_conn.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMBASE}/include/krb5

LDADD+=		-lgssapi -lkadm5srv -lhdb -lkrb5 -lhx509		\
		-lcrypto -lasn1 -lcom_err ${LIBVERS}			\
		-lroken -lcrypt -lutil
DPADD+=		${LIBGSSAPI} ${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5}	\
		${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR}	\
		${LIBVERS} ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
