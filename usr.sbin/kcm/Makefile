# $NetBSD: Makefile,v 1.1 2008/03/22 09:29:56 mlelstv Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kcm

WARNS?=  1

PROG= kcm
MAN= kcm.8

SRCS= \
	acl.c		\
	acquire.c	\
	cache.c		\
	client.c	\
	config.c	\
	connect.c	\
	cursor.c	\
	events.c	\
	glue.c		\
	headers.h	\
	kcm_locl.h	\
	kcm_protos.h	\
	log.c		\
	main.c		\
	protocol.c	\
	renew.c

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kcm			\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DIST}/heimdal/lib/asn1		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=  cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkafs -lkrb5 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT}

.include <bsd.prog.mk>
