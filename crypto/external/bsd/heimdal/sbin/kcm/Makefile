# $NetBSD: Makefile,v 1.3 2023/06/20 00:30:17 mrg Exp $

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
	-I${HEIMDIST}/lib/ipc		\
	-I${HEIMDIST}/lib

LDADD+=	-lkafs -lheimntlm -lutil
DPADD+=	${LIBKAFS} ${LIBHEIMNTLM} ${LIBUTIL}

# XXX: Pull this in from hcrypto.  Be sure to be last!
SRCS+=	ui.c
.PATH:	${HEIMDIST}/lib/hcrypto
CPPFLAGS.ui.c+= 		\
	-I${HEIMDIST}/lib/hcrypto

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
