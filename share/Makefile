#	$NetBSD: Makefile,v 1.8.2.1 2000/10/25 17:00:02 he Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk tabset termcap tmac zoneinfo
.endif

.include <bsd.subdir.mk>
