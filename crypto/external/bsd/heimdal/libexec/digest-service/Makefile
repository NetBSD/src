# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:03 christos Exp $

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

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBKDC} ${DPLIBHDB}
LDADD+=-lcrypto
DPADD+=${LIBCRYPTO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
