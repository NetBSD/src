# $NetBSD: Makefile,v 1.10 2001/12/12 00:51:07 tv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/kpasswd

WARNS?=		1

PROG=		kpasswdd
MAN=		kpasswdd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kpasswd		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H -DINETD_SUPPORT

VERS!=		cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=		-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1 \
		-lcom_err -L${VERS} -lvers -lroken -lcrypt -lutil

.include <bsd.prog.mk>
