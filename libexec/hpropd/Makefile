# $NetBSD: Makefile,v 1.11 2003/07/23 08:01:28 itojun Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kdc

WARNS?=		1

PROG=		hpropd
MAN=		hpropd.8

.if ${MKKERBEROS4} != "no"
CPPFLAGS+= -DKRB4
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
KRB4LDADD= -lkdb -lkrb
KRB4DPADD= ${LIBKDB} ${LIBKRB}
.endif

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lhdb -lkrb5 ${KRB4LDADD} -lcrypto \
		-lasn1 -lcom_err -L${VERS} -lvers -lroken \
		-lcrypt
DPADD+=		${LIBHDB} ${LIBKRB5} ${KRB4DPADD} ${LIBCRYPTO} \
		${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
		${LIBCRYPT}

.include <bsd.prog.mk>
