# $NetBSD: Makefile,v 1.16 2008/04/10 15:42:06 tsutsui Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# network server

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= kdc

SRCS+= 	524.c			\
	config.c		\
	connect.c		\
	default_config.c	\
	digest.c		\
	kaserver.c		\
	kerberos4.c		\
	kerberos5.c		\
	krb5tgs.c		\
	kx509.c			\
	log.c			\
	main.c			\
	misc.c			\
	pkinit.c		\
	process.c		\
	set_dbinfo.c		\
	windc.c

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

LDADD+=	-lhdb -lkrb5 -lhx509 -lheimntlm -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${VERS}/libvers.a ${LIBROKEN} \
	${LIBCRYPT} ${LIBUTIL}

MAN=	kdc.8

.include <bsd.prog.mk>
