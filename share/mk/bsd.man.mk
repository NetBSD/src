#	$NetBSD: bsd.man.mk,v 1.14 1994/06/30 05:31:15 cgd Exp $
#	@(#)bsd.man.mk	5.2 (Berkeley) 5/11/90

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN: all
.endif

.SUFFIXES: .0 .1 .2 .3 .4 .5 .6 .7 .8

.8.0 .7.0 .6.0 .5.0 .4.0 .3.0 .2.0 .1.0:
	@echo "nroff -mandoc ${.IMPSRC} > ${.TARGET}"
	@nroff -mandoc ${.IMPSRC} > ${.TARGET} || ( rm -f ${.TARGET} ; false )


MINSTALL=	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

maninstall:
.if defined(MAN1) && !empty(MAN1)
MANALL+=${MAN1}
maninstall: man1install
man1install:
	${MINSTALL} ${MAN1} ${DESTDIR}${MANDIR}1${MANSUBDIR}
.endif
.if defined(MAN2) && !empty(MAN2)
MANALL+=${MAN2}
maninstall: man2install
man2install:
	${MINSTALL} ${MAN2} ${DESTDIR}${MANDIR}2${MANSUBDIR}
.endif
.if defined(MAN3) && !empty(MAN3)
MANALL+=${MAN3}
maninstall: man3install
man3install:
	${MINSTALL} ${MAN3} ${DESTDIR}${MANDIR}3${MANSUBDIR}
.endif
.if defined(MAN3F) && !empty(MAN3F)
MANALL+=${MAN3F}
maninstall: man3finstall
man3finstall:
	${MINSTALL} ${MAN3F} ${DESTDIR}${MANDIR}3f${MANSUBDIR}
.endif
.if defined(MAN4) && !empty(MAN4)
MANALL+=${MAN4}
maninstall: man4install
man4install:
	${MINSTALL} ${MAN4} ${DESTDIR}${MANDIR}4${MANSUBDIR}
.endif
.if defined(MAN5) && !empty(MAN5)
MANALL+=${MAN5}
maninstall: man5install
man5install:
	${MINSTALL} ${MAN5} ${DESTDIR}${MANDIR}5${MANSUBDIR}
.endif
.if defined(MAN6) && !empty(MAN6)
MANALL+=${MAN6}
maninstall: man6install
man6install:
	${MINSTALL} ${MAN6} ${DESTDIR}${MANDIR}6${MANSUBDIR}
.endif
.if defined(MAN7) && !empty(MAN7)
MANALL+=${MAN7}
maninstall: man7install
man7install:
	${MINSTALL} ${MAN7} ${DESTDIR}${MANDIR}7${MANSUBDIR}
.endif
.if defined(MAN8) && !empty(MAN8)
MANALL+=${MAN8}
maninstall: man8install
man8install:
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
