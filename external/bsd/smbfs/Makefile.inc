#	$NetBSD: Makefile.inc,v 1.1.4.2 2014/05/22 15:51:12 yamt Exp $
#

.include <bsd.own.mk>

SMBDIST=${NETBSDSRCDIR}/external/bsd/smbfs/dist

CPPFLAGS+= -I${SMBDIST}/include
