# $NetBSD: Makefile,v 1.3 2003/07/10 11:29:41 lukem Exp $

NOMAN=

.include <bsd.own.mk>

.SUFFIXES: .src .646 .mps

BINDIR?= /usr/share/i18n/csmapper
SRCS_mapper.dir+= mapper.dir.src
SRCS_charset.pivot+= charset.pivot.src

SUBDIR= CP EBCDIC GB ISO646 ISO-8859 JIS KS MISC
.for i in ${SUBDIR}
.if exists(${.CURDIR}/$i/Makefile.inc)
.include "${.CURDIR}/$i/Makefile.inc"
.endif
.endfor

# ----------------------------------------------------------------------

mapper.dir: ${SRCS_mapper.dir}
	cat $> > $@
mapper.dir.db: mapper.dir
	${TOOL_MKCSMAPPER} -m -o $@ $>
FILES+= mapper.dir mapper.dir.db
CLEANFILES+= mapper.dir mapper.dir.db

charset.pivot: ${SRCS_charset.pivot}
	cat $> > $@
charset.pivot.pvdb: charset.pivot
	${TOOL_MKCSMAPPER} -p -o $@ $>
FILES+= charset.pivot charset.pivot.pvdb
CLEANFILES+= charset.pivot charset.pivot.pvdb

.src.mps:
	${TOOL_MKCSMAPPER} -o $@ $>

all: ${FILES}
realall: ${FILES}

.include <bsd.prog.mk>
