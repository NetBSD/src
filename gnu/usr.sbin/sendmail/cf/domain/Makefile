#	$NetBSD: Makefile,v 1.4 2003/03/24 15:04:03 atatat Exp $
#	install these sources in /usr/share/sendmail/domain

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/cf/domain

.include <Makefile.inc>

FILESDIR=	/usr/share/sendmail/domain
FILES=		${CF_DOMAIN_FILES} NetBSD.ORG.m4 ihack.net.m4 Makefile.inc

all:

.include <bsd.prog.mk>
