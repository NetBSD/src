#	$NetBSD: Makefile,v 1.2 2001/12/12 01:49:14 tv Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmdb

LIB=		smdb

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		smdb.c smdb1.c smdb2.c smndbm.c 
NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

libinstall::

.include <bsd.lib.mk>
