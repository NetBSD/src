#	$NetBSD: Makefile,v 1.10 2000/10/16 13:28:38 simonb Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk tabset termcap tmac wscons zoneinfo
.endif

.include <bsd.subdir.mk>
