#	$NetBSD: Makefile,v 1.6.4.1 2007/06/16 17:02:22 snj Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	qmgr

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c \
	qmgr_job.c qmgr_peer.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c qmgr_error.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
