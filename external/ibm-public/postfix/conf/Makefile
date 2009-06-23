#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:51 tron Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	../LICENSE ../TLS_LICENSE access aliases canonical generic header_checks \
	main.cf master.cf post-install postfix-files relocated transport \
	virtual

SCRIPTS=	postfix-script postfix-wrapper

.include <bsd.prog.mk>
