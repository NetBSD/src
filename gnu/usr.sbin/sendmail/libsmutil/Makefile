#	$NetBSD: Makefile,v 1.4 2001/12/12 12:24:21 lukem Exp $

NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		debug.c errstring.c lockfile.c safefile.c snprintf.c

libinstall::

.include <bsd.lib.mk>
