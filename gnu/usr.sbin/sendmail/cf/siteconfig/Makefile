#	$NetBSD: Makefile,v 1.4 2003/03/24 15:04:05 atatat Exp $
#	install these sources in /usr/share/sendmail/siteconfig

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/cf/siteconfig

.include <Makefile.inc>

FILESDIR=	/usr/share/sendmail/siteconfig
FILES=		${CF_SITECONFIG_FILES} Makefile.inc

all:

.include <bsd.prog.mk>
