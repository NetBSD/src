#	$NetBSD: Makefile,v 1.4 1999/02/13 07:35:52 carrel Exp $
#	install these sources in /usr/share/sendmail
#
.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=/usr/share/sendmail
FILES=README

SUBDIR=domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.files.mk>
.include <bsd.subdir.mk>

