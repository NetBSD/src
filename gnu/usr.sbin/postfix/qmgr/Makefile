#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:15 perry Exp $

PROG=	qmgr

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_rcpt_list.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
