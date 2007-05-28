# $NetBSD: Makefile,v 1.16 2007/05/28 12:06:37 tls Exp $

NOLINT= # defined

.include <bsd.own.mk>

USE_FORT?= yes	# network client

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/kadmin

WARNS?=  1

PROG= kadmin

SRCS=		 				\
	ank.c					\
	cpw.c					\
	del.c					\
	del_enctype.c				\
	dump.c					\
	ext.c					\
	get.c					\
	init.c					\
	kadmin.c				\
	load.c					\
	mod.c					\
	rename.c				\
	util.c					\
	random_password.c			\

CPPFLAGS+= -I.					\
	-I${DIST}/heimdal/kadmin		\
	-I${DIST}/heimdal/lib/krb5		\
	-I${DIST}/heimdal/lib/asn1		\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${DESTDIR}/usr/include/kadm5		\
	-I${DESTDIR}/usr/include/krb5		\
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1  \
	-lcom_err -lsl -ledit -ltermcap -L${VERS} -lvers \
	-lroken -lcrypt
DPADD+=	${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${LIBSL} ${LIBEDIT} ${LIBTERMCAP} ${VERS}/libvers.a \
	${LIBROKEN} ${LIBCRYPT}

MAN=	kadmin.8

.include <bsd.prog.mk>
