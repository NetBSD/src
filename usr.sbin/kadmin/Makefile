# $NetBSD: Makefile,v 1.13 2002/10/23 01:08:01 itojun Exp $

NOLINT= # defined

.include <bsd.own.mk>

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

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkadm5srv -lhdb -lkrb5 -lcrypto -lasn1  \
	-lcom_err -lsl -ledit -ltermcap -L${VERS} -lvers \
	-lroken -lcrypt

DPADD+=	${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${LIBSL} ${LIBEDIT} ${LIBTERMCAP} ${VERS}/libvers.a \
	${LIBROKEN} ${LIBCRYPT}

MAN=	kadmin.8

.include <bsd.prog.mk>
