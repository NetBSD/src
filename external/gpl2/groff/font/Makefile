# $NetBSD: Makefile,v 1.1 2016/01/13 19:01:59 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/font/Makefile,v 2.1 2001/04/17 12:36:42 ru Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=		devX100 devX100-12 devX75 devX75-12 \
		devascii devcp1047 devdvi devhtml devkoi8-r \
		devlatin1 devlbp devlj4 devps devutf8
.endif

.include <bsd.subdir.mk>
