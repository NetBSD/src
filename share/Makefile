#	$NetBSD: Makefile,v 1.21 2002/02/14 06:49:57 enami Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples man me misc mk sushi \
	tabset termcap tmac wscons zoneinfo
.if ${MKNLS} != "no"
SUBDIR+=locale nls
.endif
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-examples includes-man includes-sushi obj-sushi:
	@true

.include <bsd.subdir.mk>
