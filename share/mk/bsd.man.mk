#	from: @(#)bsd.man.mk	5.2 (Berkeley) 5/11/90
#	$Id: bsd.man.mk,v 1.11 1994/01/25 23:35:36 jtc Exp $

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN: all
.endif

MINSTALL=	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

maninstall:
.if defined(MAN1) && !empty(MAN1)
MANALL+=${MAN1}
maninstall: man1install
man1install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}1${MANSUBDIR}
	${MINSTALL} ${MAN1} ${DESTDIR}${MANDIR}1${MANSUBDIR}
.endif
.if defined(MAN2) && !empty(MAN2)
MANALL+=${MAN2}
maninstall: man2install
man2install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}2${MANSUBDIR}
	${MINSTALL} ${MAN2} ${DESTDIR}${MANDIR}2${MANSUBDIR}
.endif
.if defined(MAN3) && !empty(MAN3)
MANALL+=${MAN3}
maninstall: man3install
man3install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}3${MANSUBDIR}
	${MINSTALL} ${MAN3} ${DESTDIR}${MANDIR}3${MANSUBDIR}
.endif
.if defined(MAN3F) && !empty(MAN3F)
MANALL+=${MAN3F}
maninstall: man3finstall
man3finstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}3f${MANSUBDIR}
	${MINSTALL} ${MAN3F} ${DESTDIR}${MANDIR}3f${MANSUBDIR}
.endif
.if defined(MAN4) && !empty(MAN4)
MANALL+=${MAN4}
maninstall: man4install
man4install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}4${MANSUBDIR}
	${MINSTALL} ${MAN4} ${DESTDIR}${MANDIR}4${MANSUBDIR}
.endif
.if defined(MAN5) && !empty(MAN5)
MANALL+=${MAN5}
maninstall: man5install
man5install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}5${MANSUBDIR}
	${MINSTALL} ${MAN5} ${DESTDIR}${MANDIR}5${MANSUBDIR}
.endif
.if defined(MAN6) && !empty(MAN6)
MANALL+=${MAN6}
maninstall: man6install
man6install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}6${MANSUBDIR}
	${MINSTALL} ${MAN6} ${DESTDIR}${MANDIR}6${MANSUBDIR}
.endif
.if defined(MAN7) && !empty(MAN7)
MANALL+=${MAN7}
maninstall: man7install
man7install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}7${MANSUBDIR}
	${MINSTALL} ${MAN7} ${DESTDIR}${MANDIR}7${MANSUBDIR}
.endif
.if defined(MAN8) && !empty(MAN8)
MANALL+=${MAN8}
maninstall: man8install
man8install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}8${MANSUBDIR}
	${MINSTALL} ${MAN8} ${DESTDIR}${MANDIR}8${MANSUBDIR}
.endif
.if defined(MLINKS) && !empty(MLINKS)
maninstall: manlinkinstall
manlinkinstall:
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '.*\.\(.*\)'`; \
		l=$${dir}${MANSUBDIR}/`expr $$name : '\(.*\)\..*'`.0; \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '.*\.\(.*\)'`; \
		t=$${dir}${MANSUBDIR}/`expr $$name : '\(.*\)\..*'`.0; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif

.if defined(MANALL)
all: ${MANALL}

cleandir: cleanman
cleanman:
	rm -f ${MANALL}
.endif
