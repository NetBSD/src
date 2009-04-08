#	$NetBSD: Makefile,v 1.5 2009/04/08 17:24:55 christos Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no")
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

.include <bsd.subdir.mk>
