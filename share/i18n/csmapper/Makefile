# $NetBSD: Makefile,v 1.1 2003/06/27 08:40:00 tshiozak Exp $

.include <bsd.own.mk>

.SUFFIXES: .src .646 .mps

BINDIR?= /usr/share/i18n/csmapper
NOMAN=
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
	${MKCSMAPPER} -m -o $@ $>
FILES+= mapper.dir mapper.dir.db
CLEANFILES+= mapper.dir mapper.dir.db

charset.pivot: ${SRCS_charset.pivot}
	cat $> > $@
charset.pivot.pvdb: charset.pivot
	${MKCSMAPPER} -p -o $@ $>
FILES+= charset.pivot charset.pivot.pvdb
CLEANFILES+= charset.pivot charset.pivot.pvdb

.src.mps:
	${MKCSMAPPER} -o $@ $>

all: ${FILES}
realall: ${FILES}

.include <bsd.prog.mk>
