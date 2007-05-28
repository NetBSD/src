# $NetBSD: Makefile,v 1.14 2007/05/28 12:06:37 tls Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# network server

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= kdc

SRCS+= 	524.c		\
	config.c	\
	connect.c	\
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
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhdb -lkrb5 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

MAN=	kdc.8

.include <bsd.prog.mk>
