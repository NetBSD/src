#	$NetBSD: Makefile,v 1.3.22.1 2000/11/20 18:09:40 bouyer Exp $

SUBDIR=	fdesc kernfs nullfs overlay portal procfs umapfs

#
# XXX  the union LKM is currently broken.
#
.if 0
SUBDIR+= union
.endif

.include <bsd.subdir.mk>
