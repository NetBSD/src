#	$NetBSD: Makefile,v 1.15 2001/01/07 07:10:30 garbled Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples man me misc mk sushi \
	tabset termcap tmac wscons zoneinfo
.if ${MKNLS} != "no"
SUBDIR+=locale
.endif
.endif

.include <bsd.subdir.mk>
