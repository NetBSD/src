#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:40 tron Exp $
#	install these sources in /usr/share/sendmail/mailer

DIST=		${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/cf/mailer

FILESDIR=	/usr/share/sendmail/mailer
FILES=		cyrus.m4 fax.m4 local.m4 mail11.m4 phquery.m4 pop.m4 \
		procmail.m4 smtp.m4 usenet.m4 uucp.m4

all:

.include <bsd.own.mk>
.include <bsd.files.mk>
