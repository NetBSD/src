#	$NetBSD: Makefile,v 1.5 2003/03/24 15:04:04 atatat Exp $
#	install these sources in /usr/share/sendmail/mailer

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/cf/mailer

.include <Makefile.inc>

FILESDIR=	/usr/share/sendmail/mailer
FILES=		${CF_MAILER_FILES} Makefile.inc

all:

.include <bsd.prog.mk>
