# $NetBSD: Makefile,v 1.12 2002/10/23 01:06:11 itojun Exp $

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

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=		-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1 \
		-lcom_err -L${VERS} -lvers -lroken -lcrypt \
		-lutil
DPADD+=		${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
		${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} \
		${LIBUTIL}

.include <bsd.prog.mk>
