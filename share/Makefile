#	$NetBSD: Makefile,v 1.25 2006/08/30 03:20:46 yamt Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict doc examples legal man me misc mk \
	tabset termcap tmac wscons zoneinfo
.if ${MKNLS} != "no"
SUBDIR+=i18n locale nls
.endif
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-examples includes-man:
	@true

.include <bsd.subdir.mk>
