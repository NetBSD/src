#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:53 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	oqmgr

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_feedback.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c qmgr_error.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
