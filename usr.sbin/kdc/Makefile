# $NetBSD: Makefile,v 1.8 2002/10/23 01:09:37 itojun Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= kdc

SRCS= 	524.c		\
	config.c	\
	connect.c	\
	kaserver.c	\
	kdc_locl.h	\
	kerberos4.c	\
	kerberos5.c	\
	log.c		\
	main.c		\
	misc.c		\

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kdc			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/kerberosIV	\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhdb -lkrb5 -lkrb -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBHDB} ${LIBKRB5} ${LIBKRB} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

MAN=	kdc.8

.include <bsd.prog.mk>
