#	$NetBSD: Makefile,v 1.5 2002/02/02 23:21:47 perry Exp $

PROG=	master

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	master.c master_conf.c master_ent.c master_sig.c master_avail.c \
	master_spawn.c master_service.c master_status.c master_listen.c \
	master_vars.c master_wakeup.c master_flow.c

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
