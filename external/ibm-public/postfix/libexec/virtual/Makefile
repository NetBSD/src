#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:17 snj Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	virtual

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	virtual.c mailbox.c recipient.c deliver_attr.c maildir.c unknown.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
