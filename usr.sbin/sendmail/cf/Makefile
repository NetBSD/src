#	$NetBSD: Makefile,v 1.3 1999/02/13 02:55:00 lukem Exp $
#	install these sources in /usr/share/sendmail
#

.if ${MKSHARE} != "no"
FILESDIR=/usr/share/sendmail
FILES=README

SUBDIR=domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.own.mk>
.include <bsd.files.mk>
.include <bsd.subdir.mk>

