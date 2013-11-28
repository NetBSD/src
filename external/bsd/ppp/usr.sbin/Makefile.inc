#	$NetBSD: Makefile.inc,v 1.1 2013/11/28 22:33:43 christos Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>
DIST=${NETBSDSRCDIR}/external/bsd/ppp/dist
BINDIR=/usr/sbin

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif
