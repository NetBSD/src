# $NetBSD: Makefile,v 1.2.56.2 2023/10/14 06:56:34 martin Exp $

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

PROGDPLIBS+=	${DPLIBKDC} ${DPLIBHDB} ${DPLIBHEIMNTLM}

MAN=	kdc.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+=	-lutil ${KRB5LDADD} ${HDBLDADD}
DPADD+=	${LIBUTIL} ${KRB5DPADD} ${HDBDPADD}
