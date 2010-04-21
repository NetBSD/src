#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:29 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	oqmgr

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_feedback.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c qmgr_error.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
