#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:28 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	bounce

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	bounce.c bounce_append_service.c bounce_notify_service.c \
	bounce_cleanup.c bounce_notify_util.c bounce_notify_verp.c \
	bounce_one_service.c bounce_warn_service.c bounce_trace_service.c \
	bounce_template.c bounce_templates.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
