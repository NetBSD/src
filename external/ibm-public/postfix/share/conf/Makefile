#	$NetBSD: Makefile,v 1.2.4.2 2010/04/21 05:24:32 matt Exp $

.include <bsd.own.mk>

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:	${DIST}

# Not built: main.cf.default

FILES=	../LICENSE ../TLS_LICENSE access aliases canonical generic \
	header_checks main.cf master.cf relocated transport virtual

.include <bsd.prog.mk>
