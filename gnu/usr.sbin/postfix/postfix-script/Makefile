#	$NetBSD: Makefile,v 1.3 2002/09/19 03:09:49 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist/postfix/conf
.PATH:		${DIST}

# XXX Should go in /usr/libexec/postfix, but we'll need some source
# changes before we can do that for real. Meanwhile, /etc/postfix
#BINDIR=	${PFIX_LIBEXECDIR}
BINDIR=		${PFIX_ETCDIR}

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
