#	$NetBSD: Makefile,v 1.1.2.1 2001/03/09 18:02:48 he Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		debug.c errstring.c lockfile.c safefile.c snprintf.c
MKSHARE=	no
MKPIC=		no
MKPROFILE=	no

libinstall::

.include <bsd.lib.mk>
