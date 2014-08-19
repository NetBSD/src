#	$NetBSD: Makefile.inc,v 1.1.8.2 2014/08/19 23:52:12 tls Exp $
#

.include <bsd.own.mk>

SMBDIST=${NETBSDSRCDIR}/external/bsd/smbfs/dist

CPPFLAGS+= -I${SMBDIST}/include
