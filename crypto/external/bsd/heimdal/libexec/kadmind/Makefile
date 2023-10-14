# $NetBSD: Makefile,v 1.2.56.2 2023/10/14 06:56:35 martin Exp $

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

LDADD+= -lutil ${KRB5LDADD} ${HDBLDADD}
DPADD+= ${LIBUTIL} ${KRB5DPADD} ${HDBDPADD}
