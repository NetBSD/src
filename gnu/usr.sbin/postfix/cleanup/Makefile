#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:54 perry Exp $

PROG=	cleanup

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	cleanup.c cleanup_out.c cleanup_envelope.c cleanup_message.c \
	cleanup_extracted.c cleanup_state.c cleanup_skip.c cleanup_rewrite.c \
	cleanup_map11.c cleanup_map1n.c cleanup_masquerade.c \
	cleanup_out_recipient.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
