# $NetBSD: Makefile,v 1.2 2003/07/10 10:34:34 lukem Exp $

NOMAN=

.include <bsd.own.mk>

BINDIR?=	/usr/share/i18n/esdb

.SUFFIXES: .src .esdb

.src.esdb:
	${TOOL_MKESDB} -o $@ $>

SUBDIR=	CP EUC EBCDIC ISO-2022 ISO-8859 ISO646 MISC UTF

.for i in ${SUBDIR}
.include "${.CURDIR}/$i/Makefile.inc"
.endfor

# ----------------------------------------------------------------------
# esdb.{dir,alias}
#

esdb.dir: ${SRC_esdb.dir}
	cat $> > $@
esdb.dir.db: esdb.dir
	${TOOL_MKESDB} -m -o $@ $>

esdb.alias: ${SRC_esdb.alias}
	cat $> > $@
esdb.alias.db: esdb.alias
	${TOOL_MKESDB} -m -o $@ $>

FILES+=		esdb.dir esdb.dir.db esdb.alias esdb.alias.db
CLEANFILES+=	esdb.dir esdb.dir.db esdb.alias esdb.alias.db
FILESDIR:=	${BINDIR}

all: ${FILES}
realall: ${FILES}

.include <bsd.prog.mk>
