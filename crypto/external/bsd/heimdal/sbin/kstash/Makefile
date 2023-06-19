# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:45 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kdc ${HEIMDIST}/lib/hcrypto

PROG= kstash
SRCS= kstash.c ui.c
MAN=  kstash.8


CPPFLAGS+= -I${HEIMDIST}/lib/krb5 -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${HEIMDIST}/lib

#	-I${HEIMBASE}/include		\
#	-I${HEIMDIST}/base		\
#	-I${HEIMDIST}/include		\
#	-I${HEIMDIST}/lib/wind		\
#	-I${HEIMDIST}/kdc		\
#	-I${HEIMBASE}/lib/libwind	\
#	-DHAVE_CONFIG_H

LDADD+=	-lhdb
DPADD+=	${LIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
