#	$NetBSD: Makefile,v 1.2 2000/02/08 16:53:37 tron Exp $
#	install these sources in /usr/share/sendmail

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/sendmail
FILES=		LICENSE README

SUBDIR=		domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.files.mk>
.include <bsd.subdir.mk>
