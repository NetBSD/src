#	$NetBSD: Makefile,v 1.6 2010/03/20 02:07:33 uwe Exp $

#
# This Makefile exists to provide a single point to build
# all crypto libraries provided by external packages that may
# be used by other applications in the system.
#

.include <bsd.own.mk>

.if (${MKCRYPTO} != "no")
SUBDIR+= ../bsd/openssl/lib
SUBDIR+= .WAIT
SUBDIR+= ../bsd/netpgp/lib
SUBDIR+= ../bsd/openssh/lib
.endif

.include <bsd.subdir.mk>
