# $NetBSD: Makefile,v 1.13 2005/01/10 03:11:18 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kpasswd

WARNS?=		1

PROG=		kpasswdd
MAN=		kpasswdd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H -DINETD_SUPPORT

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1 \
		-lcom_err -L${VERS} -lvers -lroken -lcrypt \
		-lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
		${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} \
		${LIBUTIL}

.include <bsd.prog.mk>
