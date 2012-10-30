#	$NetBSD: Makefile,v 1.9.6.1 2012/10/30 19:00:55 yamt Exp $

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 45
WARNS?=	2	# XXX -Wcast-qual in lib/libc/time
.else
# warning with older gcc:
# "comparison is always false due to limited range of data type"
# and no way to make it shut up?
WARNS=		0
NOGCCERROR=	1
.endif

.include <bsd.own.mk>

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
