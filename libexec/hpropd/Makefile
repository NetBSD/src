# $NetBSD: Makefile,v 1.16 2008/04/10 14:26:33 tsutsui Exp $

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

LDADD+=		-lhdb -lkrb5 -lhx509 -lcrypto \
		-lasn1 -lcom_err -L${VERS} -lvers -lroken \
		-lcrypt
DPADD+=		${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} \
		${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
		${LIBCRYPT}

.include <bsd.prog.mk>
