#	$NetBSD: Makefile,v 1.8 1999/02/13 02:54:34 lukem Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk skel tabset termcap tmac zoneinfo
.endif

.include <bsd.subdir.mk>
