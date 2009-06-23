#	$NetBSD: Makefile,v 1.2 2009/06/23 17:55:06 tron Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:		${DIST}

BINDIR=		${PFIX_LIBEXECDIR}

SCRIPTS=	post-install postfix-script postfix-wrapper postmulti-script

FILES=		postfix-files
FILESDIR=	${PFIX_LIBEXECDIR}

.include <bsd.prog.mk>
