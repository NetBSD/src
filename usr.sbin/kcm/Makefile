# $NetBSD: Makefile,v 1.2.10.1 2009/05/13 19:20:27 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wextra -Wsign-compare
NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kcm

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

LDADD+=	-lkafs -lkrb5 -lhx509 -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <bsd.prog.mk>
