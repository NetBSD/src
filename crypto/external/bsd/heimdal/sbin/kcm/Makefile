# $NetBSD: Makefile,v 1.2.56.1 2023/08/11 13:40:09 martin Exp $

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

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

LDADD+=	-lutil ${KRB5LDADD}
DPADD+=	${LIBCRYPTO} ${KRB5DPADD}
