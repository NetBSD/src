#	$NetBSD: Makefile,v 1.2 2002/02/02 23:21:48 perry Exp $

DIST=		${.CURDIR}/../../../dist/postfix/conf
.PATH:		${DIST}

# XXX Should go in /usr/libexec/postfix, but we'll need some source
# changes before we can do that for real. Meanwhile, /etc/postfix
#BINDIR=	${PFIX_LIBEXECDIR}
BINDIR=		${PFIX_ETCDIR}

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
