#	$NetBSD: Makefile,v 1.2 2000/12/26 13:11:20 itojun Exp $

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
