# $NetBSD: Makefile,v 1.10 2005/01/10 03:11:18 lukem Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= hprop

.if ${MKKERBEROS4} != "no"
CPPFLAGS+= -DKRB4
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
KRB4LDADD= -lkdb -lkrb
KRB4DPADD= ${LIBKDB} ${LIBKRB}
.endif


SRCS+= 	hprop.c					\
	v4_dump.c				\
	mit_dump.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${DIST}/heimdal/lib/hdb		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhdb -lkrb5 ${KRB4LDADD} -lcrypto -lasn1 \
	-lcom_err -L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBHDB} ${LIBKRB5} ${KRB4DPADD} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

MAN=	hprop.8

.include <bsd.prog.mk>
