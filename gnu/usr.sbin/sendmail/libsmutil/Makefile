#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:14 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		debug.c errstring.c lockfile.c safefile.c snprintf.c
NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

libinstall::

.include <bsd.lib.mk>
