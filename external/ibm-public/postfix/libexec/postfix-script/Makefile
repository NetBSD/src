#	$NetBSD: Makefile,v 1.1.60.1 2024/01/14 13:02:26 martin Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:		${DIST}

SCRIPTS=	post-install postfix-script postfix-tls-script \
		postfix-wrapper postmulti-script

FILES=		postfix-files
FILESDIR=	${PFIX_LIBEXECDIR}

.include <bsd.prog.mk>
