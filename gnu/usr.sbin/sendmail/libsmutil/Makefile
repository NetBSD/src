#	$NetBSD: Makefile,v 1.1 2000/05/03 10:50:59 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		debug.c errstring.c lockfile.c safefile.c snprintf.c strl.c
MKSHARE=	no
MKPIC=		no
MKPROFILE=	no

libinstall::

.include <bsd.lib.mk>
