#	@(#)bsd.man.mk	5.2 (Berkeley) 5/11/90

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

MANGRP?=	bin
MANOWN?=	bin
MANMODE?=	444

MANDIR?=	/usr/share/man/cat

MINSTALL=	install -c -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

maninstall:
.if defined(MAN1) && !empty(MAN1)
	@if [ ! -d ${DESTDIR}${MANDIR}1${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}1${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}1${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}1${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}1${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN1} ${DESTDIR}${MANDIR}1${MANSUBDIR}
.endif
.if defined(MAN2) && !empty(MAN2)
	@if [ ! -d ${DESTDIR}${MANDIR}2${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}2${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}2${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}2${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}2${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN2} ${DESTDIR}${MANDIR}2${MANSUBDIR}
.endif
.if defined(MAN3) && !empty(MAN3)
	@if [ ! -d ${DESTDIR}${MANDIR}3${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}3${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}3${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}3${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}3${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN3} ${DESTDIR}${MANDIR}3${MANSUBDIR}
.endif
.if defined(MAN3F) && !empty(MAN3F)
	@if [ ! -d ${DESTDIR}${MANDIR}3f${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}3f${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}3f${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}3f${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}3f${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN3F} ${DESTDIR}${MANDIR}3f${MANSUBDIR}
.endif
.if defined(MAN4) && !empty(MAN4)
	@if [ ! -d ${DESTDIR}${MANDIR}4${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}4${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}4${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}4${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}4${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN4} ${DESTDIR}${MANDIR}4${MANSUBDIR}
.endif
.if defined(MAN5) && !empty(MAN5)
	@if [ ! -d ${DESTDIR}${MANDIR}5${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}5${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}5${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}5${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}5${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN5} ${DESTDIR}${MANDIR}5${MANSUBDIR}
.endif
.if defined(MAN6) && !empty(MAN6)
	@if [ ! -d ${DESTDIR}${MANDIR}6${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}6${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}6${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}6${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}6${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN6} ${DESTDIR}${MANDIR}6${MANSUBDIR}
.endif
.if defined(MAN7) && !empty(MAN7)
	@if [ ! -d ${DESTDIR}${MANDIR}7${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}7${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}7${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}7${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}7${MANSUBDIR} ; \
        else \
                true ; \
        fi
	${MINSTALL} ${MAN7} ${DESTDIR}${MANDIR}7${MANSUBDIR}
.endif
.if defined(MAN8) && !empty(MAN8)
	@if [ ! -d ${DESTDIR}${MANDIR}8${MANSUBDIR} ]; then \
                /bin/rm -f ${DESTDIR}${MANDIR}8${MANSUBDIR} ; \
                mkdir -p ${DESTDIR}${MANDIR}8${MANSUBDIR} ; \
                chown root.wheel ${DESTDIR}${MANDIR}8${MANSUBDIR} ; \
                chmod 755 ${DESTDIR}${MANDIR}8${MANSUBDIR} ; \
        else \
                true ; \
        fi
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
