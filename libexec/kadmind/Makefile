# $NetBSD: Makefile,v 1.11 2002/08/19 13:54:35 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/kadmin

WARNS?=		1

PROG=		kadmind
MAN=		kadmind.8

SRCS= 	kadmind.c				\
	server.c				\
	version4.c				\
	kadm_conn.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kadmin		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=		cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD=		-lkadm5srv -lhdb -lkrb5 -lkadm -lkrb -lcrypto \
		-lasn1 -lcom_err -L${VERS} -lvers -lroken -lcrypt -lutil

.include <bsd.prog.mk>
