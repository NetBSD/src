#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:44 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	cleanup

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	cleanup.c cleanup_out.c cleanup_envelope.c cleanup_message.c \
	cleanup_extracted.c cleanup_state.c cleanup_rewrite.c \
	cleanup_map11.c cleanup_map1n.c cleanup_masquerade.c \
	cleanup_out_recipient.c cleanup_init.c cleanup_api.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
