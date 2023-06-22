# $NetBSD: Makefile,v 1.7 2023/06/22 22:39:37 christos Exp $

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

PROGDPLIBS+= ${DPLIBGSSAPI} ${DPLIBKADM5SRV} ${DPLIBHDB} ${DPLIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= -lutil ${KRB5LDADD}
DPADD+= ${LIBUTIL} ${KRB5DPADD}
