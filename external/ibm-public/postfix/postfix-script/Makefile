#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:54 tron Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:		${DIST}

# XXX Should go in /usr/libexec/postfix, but we'll need some source
# changes before we can do that for real. Meanwhile, /etc/postfix
#BINDIR=	${PFIX_LIBEXECDIR}
BINDIR=		${PFIX_ETCDIR}

SCRIPTS=	postfix-script postfix-wrapper

.include <bsd.prog.mk>
