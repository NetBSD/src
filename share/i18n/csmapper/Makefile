# $NetBSD: Makefile,v 1.4 2003/10/19 14:08:50 lukem Exp $

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
	${_MKMSGCREATE}
	${_MKCMD}\
	cat $> > $@
mapper.dir.db: mapper.dir
	${_MKMSGCREATE}
	${_MKCMD}\
	${TOOL_MKCSMAPPER} -m -o $@ $>
FILES+= mapper.dir mapper.dir.db
CLEANFILES+= mapper.dir mapper.dir.db

charset.pivot: ${SRCS_charset.pivot}
	${_MKMSGCREATE}
	${_MKCMD}\
	cat $> > $@
charset.pivot.pvdb: charset.pivot
	${_MKMSGCREATE}
	${_MKCMD}\
	${TOOL_MKCSMAPPER} -p -o $@ $>
FILES+= charset.pivot charset.pivot.pvdb
CLEANFILES+= charset.pivot charset.pivot.pvdb

.src.mps:
	${_MKMSGCREATE}
	${_MKCMD}\
	${TOOL_MKCSMAPPER} -o $@ $>

all: ${FILES}
realall: ${FILES}

.include <bsd.prog.mk>
