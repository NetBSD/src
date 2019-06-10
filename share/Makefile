#	$NetBSD: Makefile,v 1.35.2.1 2019/06/10 22:05:37 christos Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict examples installboot legal man me misc mk \
	tabset terminfo tmac wscons xml

.if ${MKGROFF} != "no"
SUBDIR+=doc
.endif

.if ${MKNLS} != "no"
SUBDIR+=i18n locale nls
.endif
.endif

SUBDIR.roff+= doc

# Speedup stubs for some subtrees that don't need to run these rules
includes-examples includes-man:
	@true

.include <bsd.subdir.mk>
