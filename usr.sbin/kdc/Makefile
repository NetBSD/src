# $NetBSD: Makefile,v 1.10 2003/08/23 23:03:45 lha Exp $

NOLINT= # defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kdc

WARNS?=  1

PROG= kdc

.if ${MKKERBEROS4} != "no"
CPPFLAGS+= -DKRB4
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
KRB4LDADD= -lkrb -ldes
KRB4DPADD= ${LIBKRB} ${LIBDES}

SRCS+= 	kaserver.c

.endif

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

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lhdb -lkrb5 ${KRB4LDADD} -lcrypto -lasn1 -lcom_err \
	-L${VERS} -lvers -lroken -lcrypt -lutil
DPADD+=	${LIBHDB} ${LIBKRB5} ${KRB4DPADD} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} \
	${VERS}/libvers.a ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

MAN=	kdc.8

.include <bsd.prog.mk>
