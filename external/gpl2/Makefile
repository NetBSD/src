#	$NetBSD: Makefile,v 1.4 2009/04/08 16:30:12 christos Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no"
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

.include <bsd.subdir.mk>
