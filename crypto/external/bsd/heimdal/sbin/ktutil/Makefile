# $NetBSD: Makefile,v 1.1 2011/04/13 19:17:00 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/admin

PROG= ktutil

MAN= ktutil.8

HEIMSRCS= ktutil-commands.in

SRCS=			\
	add.c		\
	change.c	\
	copy.c		\
	destroy.c	\
	get.c		\
	ktutil.c	\
	ktutil_locl.h	\
	list.c		\
	purge.c		\
	remove.c	\
	rename.c

LDADD+=	-lkadm5clnt -lkrb5 -lsl ${LIBVERS} -lroken -lutil
DPADD+=	${LIBKADM5CLNT} ${LIBKRB5} ${LIBCRYPTO} \
	${LIBASN1} ${LIBCOM_ERR} ${LIBSL} ${LIBEDIT} ${LIBTERMINFO} \
	${LIBVERS} ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
