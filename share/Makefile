#	$NetBSD: Makefile,v 1.8.10.1 2000/08/09 07:46:22 drochner Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk skel tabset termcap tmac wscons \
	zoneinfo
.endif

.include <bsd.subdir.mk>
