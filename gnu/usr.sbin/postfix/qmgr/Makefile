#	$NetBSD: Makefile,v 1.5 2004/04/27 05:33:55 heas Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	qmgr

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_rcpt_list.c \
	qmgr_job.c qmgr_peer.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
