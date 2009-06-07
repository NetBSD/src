#	$NetBSD: Makefile,v 1.5 2009/06/07 22:59:22 christos Exp $

#
# This Makefile exists to provide a single point to build
# all compat libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= ../bsd/openssh/lib
.endif

.include <bsd.subdir.mk>
