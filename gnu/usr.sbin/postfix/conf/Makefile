#	$NetBSD: Makefile,v 1.9 2005/08/18 22:30:37 rpaulo Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	LICENSE TLS_LICENSE access aliases canonical generic header_checks \
	main.cf master.cf post-install postfix-files relocated transport \
	virtual

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
