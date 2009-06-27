#	$NetBSD: Makefile,v 1.2 2009/06/27 21:12:12 tron Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	../LICENSE ../TLS_LICENSE access aliases canonical generic \
	header_checks main.cf master.cf relocated transport virtual

.include <bsd.prog.mk>
