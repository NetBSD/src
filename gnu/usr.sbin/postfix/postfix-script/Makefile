#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:11 perry Exp $

DIST=		${.CURDIR}/../../../dist/postfix/conf
.PATH:		${DIST}

# XXX Should go in /usr/libexec/postfix, but we'll need some source
# changes before we can do that for real. Meanwhile, /etc/postfix
#BINDIR=	${PFIX_LIBEXECDIR}
BINDIR=		${PFIX_ETCDIR}

SCRIPTS=	postfix-script-sgid
SCRIPTSNAME=	postfix-script

.include <bsd.prog.mk>
