#	$NetBSD: Makefile,v 1.7 1998/09/27 16:31:39 lukem Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if !defined(NOSHARE) || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk skel tabset termcap tmac zoneinfo
.endif

.include <bsd.subdir.mk>
