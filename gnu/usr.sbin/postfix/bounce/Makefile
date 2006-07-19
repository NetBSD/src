#	$NetBSD: Makefile,v 1.8 2006/07/19 02:04:43 rpaulo Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	bounce

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	bounce.c bounce_append_service.c bounce_notify_service.c \
	bounce_cleanup.c bounce_notify_util.c bounce_notify_verp.c \
	bounce_one_service.c bounce_warn_service.c bounce_trace_service.c \
	bounce_template.c bounce_templates.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
