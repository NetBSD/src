#	$NetBSD: Makefile,v 1.2 2002/09/19 03:09:52 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	virtual

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	virtual.c mailbox.c recipient.c deliver_attr.c maildir.c unknown.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
