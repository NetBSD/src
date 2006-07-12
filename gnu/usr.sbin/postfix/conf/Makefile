#	$NetBSD: Makefile,v 1.8.2.1 2006/07/12 15:06:48 tron Exp $

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
