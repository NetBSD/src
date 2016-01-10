#	$NetBSD: Makefile,v 1.6 2016/01/10 22:19:41 christos Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no")
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

SUBDIR+= grep

.include <bsd.subdir.mk>
