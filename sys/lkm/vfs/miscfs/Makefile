#	$NetBSD: Makefile,v 1.3 1997/02/18 06:08:51 scottr Exp $

SUBDIR=	fdesc kernfs nullfs portal procfs umapfs

#
# XXX  the union LKM is currently broken.
#
.if 0
SUBDIR+= union
.endif

.include <bsd.subdir.mk>
