#	$NetBSD: Makefile,v 1.5 2002/12/24 19:47:52 perry Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtp

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	smtp.c smtp_connect.c smtp_proto.c smtp_chat.c smtp_session.c \
	smtp_addr.c smtp_trouble.c smtp_state.c \
	smtp_sasl_proto.c smtp_sasl_glue.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

.include <bsd.prog.mk>
