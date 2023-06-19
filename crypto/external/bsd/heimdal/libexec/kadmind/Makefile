# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kadmin ${HEIMDIST}/lib/hcrypto

PROG=	kadmind
MAN=	kadmind.8

SRCS+= 	kadmind.c	\
	rpc.c		\
	server.c	\
	kadm_conn.c	\
	ui.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMBASE}/include/krb5

LDADD+= -lgssapi -lkadm5srv -lhdb -lutil
DPADD+= ${LIBGSSAPI} ${LIBKADM5SRV} ${LIBHDB} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

CLEANFILES+=hcrypto

.BEGIN:
	@ln -sf ${HEIMDIST}/lib/hcrypto .
