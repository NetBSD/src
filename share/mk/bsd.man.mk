#	from: @(#)bsd.man.mk	5.2 (Berkeley) 5/11/90
#	$Id: bsd.man.mk,v 1.7 1993/08/15 19:37:07 mycroft Exp $

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

MANGRP?=	bin
MANOWN?=	bin
MANMODE?=	444

MANDIR?=	/usr/share/man/cat

MINSTALL=	install ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

maninstall:
.if defined(MAN1) && !empty(MAN1)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}1${MANSUBDIR}
	${MINSTALL} ${MAN1} ${DESTDIR}${MANDIR}1${MANSUBDIR}
.endif
.if defined(MAN2) && !empty(MAN2)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}2${MANSUBDIR}
	${MINSTALL} ${MAN2} ${DESTDIR}${MANDIR}2${MANSUBDIR}
.endif
.if defined(MAN3) && !empty(MAN3)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}3${MANSUBDIR}
	${MINSTALL} ${MAN3} ${DESTDIR}${MANDIR}3${MANSUBDIR}
.endif
.if defined(MAN3F) && !empty(MAN3F)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}3f${MANSUBDIR}
	${MINSTALL} ${MAN3F} ${DESTDIR}${MANDIR}3f${MANSUBDIR}
.endif
.if defined(MAN4) && !empty(MAN4)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}4${MANSUBDIR}
	${MINSTALL} ${MAN4} ${DESTDIR}${MANDIR}4${MANSUBDIR}
.endif
.if defined(MAN5) && !empty(MAN5)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}5${MANSUBDIR}
	${MINSTALL} ${MAN5} ${DESTDIR}${MANDIR}5${MANSUBDIR}
.endif
.if defined(MAN6) && !empty(MAN6)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}6${MANSUBDIR}
	${MINSTALL} ${MAN6} ${DESTDIR}${MANDIR}6${MANSUBDIR}
.endif
.if defined(MAN7) && !empty(MAN7)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}7${MANSUBDIR}
	${MINSTALL} ${MAN7} ${DESTDIR}${MANDIR}7${MANSUBDIR}
.endif
.if defined(MAN8) && !empty(MAN8)
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MANDIR}8${MANSUBDIR}
	${MINSTALL} ${MAN8} ${DESTDIR}${MANDIR}8${MANSUBDIR}
.endif
.if defined(MLINKS) && !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '[^\.]*\.\(.*\)'`; \
		l=$${dir}${MANSUBDIR}/`expr $$name : '\([^\.]*\)'`.0; \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}`expr $$name : '[^\.]*\.\(.*\)'`; \
		t=$${dir}${MANSUBDIR}/`expr $$name : '\([^\.]*\)'`.0; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif
