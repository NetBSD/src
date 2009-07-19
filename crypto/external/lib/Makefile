#	$NetBSD: Makefile,v 1.5 2009/07/19 23:44:20 christos Exp $

#
# This Makefile exists to provide a single point to build
# all crypto libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= ../bsd/netpgp/lib
SUBDIR+= ../bsd/openssh/lib
SUBDIR+= ../bsd/openssl/lib
.endif

.include <bsd.subdir.mk>
