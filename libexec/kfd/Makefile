# $NetBSD: Makefile,v 1.8 2001/12/12 00:51:07 tv Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH:		${DIST}/heimdal/appl/kf

WARNS?=		1

PROG=		kfd
MAN=		kfd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/appl/kf		\
	-I${.CURDIR}/../../include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=		cd ${.CURDIR}/../../lib/libvers && ${PRINTOBJDIR}

LDADD=		-lkrb5 -lcrypto -lasn1 -lcom_err \
		-L${VERS} -lvers -lroken -lcrypt

.include <bsd.prog.mk>
