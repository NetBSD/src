#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:08 tv Exp $

PROG=	master

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	master.c master_conf.c master_ent.c master_sig.c master_avail.c \
	master_spawn.c master_service.c master_status.o master_listen.c \
	master_proto.c single_server.c multi_server.c master_vars.c \
	master_wakeup.c

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
