#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:12 tv Exp $

PROG=	qmgr

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmgr.c qmgr_active.c qmgr_transport.c qmgr_queue.c qmgr_entry.c \
	qmgr_message.c qmgr_deliver.c qmgr_move.c qmgr_rcpt_list.c \
	qmgr_defer.c qmgr_enable.c qmgr_scan.c qmgr_bounce.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
