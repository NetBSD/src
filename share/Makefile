#	$NetBSD: Makefile,v 1.12 2000/12/22 01:31:47 itojun Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples man me misc mk mklocale \
	tabset termcap tmac wscons zoneinfo
.endif

.include <bsd.subdir.mk>
