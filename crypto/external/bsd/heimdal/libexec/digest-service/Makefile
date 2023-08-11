# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:07 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH:		${HEIMDIST}/kdc

PROG=	digest-service
SRCS=	digest-service.c
MAN=

CPPFLAGS+= 				\
	-I${HEIMBASE}/include/krb5	\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/ipc

PROGDPLIBS+= ${DPLIBKDC} ${DPLIBHDB} ${DPLIBHEIMNTLM}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+= ${KRB5LDADD}
DPADD+= ${KRB5DPADD}
