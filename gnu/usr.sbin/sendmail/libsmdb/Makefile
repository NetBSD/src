#	$NetBSD: Makefile,v 1.1 2000/05/03 10:50:58 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmdb

LIB=		smdb

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		smdb.c smdb1.c smdb2.c smndbm.c 
MKSHARE=	no
MKPIC=		no
MKPROFILE=	no

libinstall::

.include <bsd.lib.mk>
