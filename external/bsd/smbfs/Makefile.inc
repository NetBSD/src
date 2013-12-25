#	$NetBSD: Makefile.inc,v 1.1 2013/12/25 22:03:14 christos Exp $
#

.include <bsd.own.mk>

SMBDIST=${NETBSDSRCDIR}/external/bsd/smbfs/dist

CPPFLAGS+= -I${SMBDIST}/include
