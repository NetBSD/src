#	$NetBSD: Makefile,v 1.8 2004/04/27 05:33:55 heas Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	LICENSE access aliases canonical header_checks main.cf master.cf \
	post-install postfix-files relocated transport virtual

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
