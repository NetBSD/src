#	$NetBSD: Makefile,v 1.11 2000/12/11 19:39:33 augustss Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples man me misc mk tabset termcap tmac wscons zoneinfo
.endif

.include <bsd.subdir.mk>
