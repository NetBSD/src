#	$NetBSD: Makefile,v 1.2.6.1 1997/03/12 21:24:13 is Exp $

SUBDIR=	fdesc kernfs nullfs portal procfs umapfs

#
# XXX  the union LKM is currently broken.
#
.if 0
SUBDIR+= union
.endif

.include <bsd.subdir.mk>
