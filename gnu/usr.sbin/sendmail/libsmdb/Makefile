#	$NetBSD: Makefile,v 1.1.2.1 2002/02/20 23:15:58 he Exp $

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
