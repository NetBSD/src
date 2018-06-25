#	$NetBSD: Makefile,v 1.33.22.1 2018/06/25 07:25:36 pgoyette Exp $
#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

# Missing:  ms

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	dict examples legal man me misc mk \
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
