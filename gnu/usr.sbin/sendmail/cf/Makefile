#	$NetBSD: Makefile,v 1.5 2002/09/19 03:09:52 lukem Exp $
#	install these sources in /usr/share/sendmail

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/cf ${DIST}/sendmail

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/sendmail
FILES=		LICENSE README

SUBDIR=		domain feature hack m4 mailer ostype sh siteconfig
.endif

all:

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
