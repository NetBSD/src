#	$NetBSD: Makefile,v 1.4 2000/01/20 19:06:15 wrstuden Exp $

SUBDIR=	fdesc kernfs nullfs overlay portal procfs umapfs

#
# XXX  the union LKM is currently broken.
#
.if 0
SUBDIR+= union
.endif

.include <bsd.subdir.mk>
