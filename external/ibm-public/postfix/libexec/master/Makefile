#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:29 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	master

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	master.c master_conf.c master_ent.c master_sig.c master_avail.c \
	master_spawn.c master_service.c master_status.c master_listen.c \
	master_vars.c master_wakeup.c master_flow.c master_watch.c

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
