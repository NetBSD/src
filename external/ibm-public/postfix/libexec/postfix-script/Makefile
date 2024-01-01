#	$NetBSD: Makefile,v 1.2 2024/01/01 18:56:53 kim Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/ibm-public/postfix/dist/conf
.PATH:		${DIST}

SCRIPTS=	post-install postfix-script postfix-tls-script \
		postfix-wrapper postmulti-script

FILES=		postfix-files
FILESDIR=	${PFIX_LIBEXECDIR}

.include <bsd.prog.mk>
