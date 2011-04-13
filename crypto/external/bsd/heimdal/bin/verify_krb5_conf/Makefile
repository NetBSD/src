#	$NetBSD: Makefile,v 1.1 2011/04/13 19:16:47 elric Exp $

WARNS?=	2	# XXX -Wextra

NOLINT= # defined

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/krb5

PROG= verify_krb5_conf
MAN= verify_krb5_conf.8

SRCS= verify_krb5_conf.c

#CPPFLAGS+= -I.					\
#	-I${DIST}/heimdal/lib/asn1		\
#	-I${NETBSDSRCDIR}/include/heimdal	\
#	-I${DESTDIR}/usr/include/krb5		\
#	-DHAVE_CONFIG_H

LDADD+=	-lkrb5 -lhx509 -lcrypto -lasn1 -lcom_err \
	${LIBVERS} -lroken -lcrypt
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${LIBVERS} ${LIBROKEN} ${LIBCRYPT}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
