#	$NetBSD: Makefile,v 1.5 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libsmdb

LIB=		smdb

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		smdb.c smdb1.c smdb2.c smndbm.c 

.include <bsd.lib.mk>
