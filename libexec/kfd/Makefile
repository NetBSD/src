# $NetBSD: Makefile,v 1.9 2002/08/19 13:54:35 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/appl/kf

WARNS?=		1

PROG=		kfd
MAN=		kfd.8

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/appl/kf		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD=		-lkrb5 -lcrypto -lasn1 -lcom_err \
		-L${VERS} -lvers -lroken -lcrypt

.include <bsd.prog.mk>
