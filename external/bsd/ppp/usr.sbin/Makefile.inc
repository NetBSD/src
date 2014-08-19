#	$NetBSD: Makefile.inc,v 1.1.8.2 2014/08/19 23:52:12 tls Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

USE_FORT?=yes	# network client/server

.include <bsd.own.mk>
DIST=${NETBSDSRCDIR}/external/bsd/ppp/dist
BINDIR=/usr/sbin

.if defined(HAVE_GCC) || defined(HAVE_LLVM)
COPTS+=	-fno-strict-aliasing
.endif
