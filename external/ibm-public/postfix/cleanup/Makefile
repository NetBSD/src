#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:51 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	cleanup

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	cleanup.c cleanup_out.c cleanup_envelope.c cleanup_message.c \
	cleanup_extracted.c cleanup_state.c cleanup_rewrite.c \
	cleanup_map11.c cleanup_map1n.c cleanup_masquerade.c \
	cleanup_out_recipient.c cleanup_init.c cleanup_api.c \
	cleanup_addr.c cleanup_bounce.c cleanup_milter.c \
	cleanup_body_edit.c cleanup_region.c cleanup_final.c

DPADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
