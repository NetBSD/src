#	$NetBSD: Makefile,v 1.1 2002/02/02 23:21:48 perry Exp $

PROG=	nqmgr

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_rcpt_list.c \
	qmgr_job.c qmgr_peer.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
