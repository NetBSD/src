#	$NetBSD: Makefile,v 1.8.8.1 2000/05/28 22:40:55 minoura Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples lkm man me misc mk skel tabset termcap tmac zoneinfo
.endif
SUBDIR+= colldef mklocale timedef

.include <bsd.subdir.mk>
