#	$NetBSD: Makefile,v 1.9 2000/07/25 17:18:33 drochner Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk skel tabset termcap tmac wscons \
	zoneinfo
.endif

.include <bsd.subdir.mk>
