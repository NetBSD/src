#	$NetBSD: Makefile,v 1.7 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		cf.c debug.c err.c lockfile.c safefile.c snprintf.c

.include <bsd.lib.mk>
