#	$NetBSD: Makefile,v 1.2 1998/09/27 17:22:14 lukem Exp $
#	install these sources in /usr/share/sendmail
#

.if !defined(NOSHARE)
FILESDIR=/usr/share/sendmail
FILES=README

SUBDIR=domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.own.mk>
.include <bsd.files.mk>
.include <bsd.subdir.mk>

