#	$NetBSD: Makefile,v 1.10 2008/08/30 23:14:38 erh Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	../LICENSE ../TLS_LICENSE access aliases canonical generic header_checks \
	main.cf master.cf post-install postfix-files relocated transport \
	virtual

SCRIPTS=	postfix-script

.include <bsd.prog.mk>
