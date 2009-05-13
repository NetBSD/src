#	$NetBSD: Makefile,v 1.3.2.1 2009/05/13 18:52:39 jym Exp $

.include <bsd.own.mk>

.if (${MKCVS} != "no")
SUBDIR+= xcvs
.endif
.if (${MKLVM} != "no")
SUBDIR+= lvm2
.endif

.include <bsd.subdir.mk>
