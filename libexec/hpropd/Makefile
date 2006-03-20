# $NetBSD: Makefile,v 1.14 2006/03/20 04:03:22 christos Exp $

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
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lhdb -lkrb5 ${KRB4LDADD} -lcrypto \
		-lasn1 -lcom_err -L${VERS} -lvers -lroken \
		-lcrypt
DPADD+=		${LIBHDB} ${LIBKRB5} ${KRB4DPADD} ${LIBCRYPTO} \
		${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
		${LIBCRYPT}

.include <bsd.prog.mk>
