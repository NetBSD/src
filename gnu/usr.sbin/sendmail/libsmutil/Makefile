#	$NetBSD: Makefile,v 1.1.2.2 2002/02/20 23:16:00 he Exp $

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
