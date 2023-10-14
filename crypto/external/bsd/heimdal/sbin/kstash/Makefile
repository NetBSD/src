# $NetBSD: Makefile,v 1.2.56.2 2023/10/14 06:56:34 martin Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# cryptographic software

.PATH: ${HEIMDIST}/kdc ${HEIMDIST}/lib/hcrypto

PROG= kstash
SRCS= kstash.c ui.c
MAN=  kstash.8


CPPFLAGS+= -I${HEIMDIST}/lib/krb5 -I${DESTDIR}/usr/include/krb5

#	-I${HEIMBASE}/include		\
#	-I${HEIMDIST}/base		\
#	-I${HEIMDIST}/include		\
#	-I${HEIMDIST}/lib/wind		\
#	-I${HEIMDIST}/kdc		\
#	-I${HEIMBASE}/lib/libwind	\
#	-DHAVE_CONFIG_H

PROGDPLIBS+= ${DPLIBHDB}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+=${KRB5LDADD} ${HDBLDADD}
DPADD+=${KRB5DPADD} ${HDBDPADD}
