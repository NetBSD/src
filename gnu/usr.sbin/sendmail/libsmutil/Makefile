#	$NetBSD: Makefile,v 1.6 2003/03/24 15:04:07 atatat Exp $

NOSHARE=	# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libsmutil

LIB=		smutil

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		cf.c debug.c err.c lockfile.c safefile.c snprintf.c

libinstall::

.include <bsd.lib.mk>
