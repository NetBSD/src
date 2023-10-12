# $NetBSD: Makefile,v 1.6 2023/10/12 17:18:38 riastradh Exp $

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

LDADD+= ${KRB5LDADD} ${HDBLDADD}
DPADD+= ${KRB5DPADD} ${HDBDPADD}
