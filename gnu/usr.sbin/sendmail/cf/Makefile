#	$NetBSD: Makefile,v 1.4 2002/04/24 08:18:47 lukem Exp $
#	install these sources in /usr/share/sendmail

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/cf ${DIST}/sendmail

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/sendmail
FILES=		LICENSE README

SUBDIR=		domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
