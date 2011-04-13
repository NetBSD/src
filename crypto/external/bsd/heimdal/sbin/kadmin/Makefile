# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:59 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/kadmin

PROG=	kadmin
MAN=	kadmin.8

HEIMSRCS= kadmin-commands.in

SRCS=	add_enctype.c				\
	ank.c					\
	check.c					\
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
	pw_quality.c				\
	random_password.c			\
	rename.c				\
	stash.c					\
	util.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/base		\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib/wind		\
	-I${HEIMBASE}/lib/libwind	\
	-I${DESTDIR}/usr/include/kadm5

LDADD+=	-lkadm5srv -lkadm5clnt -lkrb5 -lsl -ledit ${LIBVERS} -lroken -lutil
DPADD+=	${LIBKADM5SRV} ${LIBHDB} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${LIBSL} ${LIBEDIT} ${LIBTERMINFO} \
	${LIBVERS} ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
