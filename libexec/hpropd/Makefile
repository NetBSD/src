# $NetBSD: Makefile,v 1.10 2002/10/23 01:15:57 itojun Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kdc

WARNS?=		1

PROG=		hpropd
MAN=		hpropd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lhdb -lkrb5 -lkdb -lkrb -lcrypto \
		-lasn1 -lcom_err -L${VERS} -lvers -lroken \
		-lcrypt
DPADD+=		${LIBHDB} ${LIBKRB5} ${LIBKDB} ${LIBKRB} ${LIBCRYPTO} \
		${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
		${LIBCRYPT

.include <bsd.prog.mk>
