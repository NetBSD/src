# $NetBSD: Makefile,v 1.1 2011/04/13 19:16:59 elric Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/kcm

PROG= kcm
MAN=  kcm.8

NOGCCERROR=	# Heimdal is using its own deprecated functions

SRCS= \
	acl.c		\
	acquire.c	\
	cache.c		\
	client.c	\
	config.c	\
	connect.c	\
	events.c	\
	glue.c		\
	log.c		\
	main.c		\
	protocol.c	\
	renew.c		\
	sessions.c

CPPFLAGS+= -I${HEIMDIST}/lib/ipc

LDADD+=	-lkafs -lkrb5 -lheimntlm -lhx509 -lcrypto -lasn1 -lcom_err \
	${LIBIPC_PIC} ${LIBVERS} -lroken -lcrypt -lutil
DPADD+=	${LIBKAFS} ${LIBKRB5} ${LIBHX509} ${LIBCRYPTO} ${LIBASN1} \
	${LIBCOM_ERR} ${LIBVERS} ${LIBROKEN} ${LIBCRYPT} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
