# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:04 christos Exp $

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

CPPFLAGS+=				\
	-I${HEIMDIST}/lib/ipc

PROGDPLIBS+=	${DPLIBKAFS} ${DPLIBHEIMNTLM}
LDADD+=	-lcrypto -lutil
DPADD+=	${LIBCRYPTO} ${LIBUTIL}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
