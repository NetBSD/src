#	$NetBSD: Makefile,v 1.3 2001/12/12 12:24:20 lukem Exp $

NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/libsmdb

LIB=		smdb

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		smdb.c smdb1.c smdb2.c smndbm.c 

libinstall::

.include <bsd.lib.mk>
